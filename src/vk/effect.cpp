#include "effect.hpp"

#include <cstring>
#include <filesystem>
#include <stdexcept>

#include <effect_parser.hpp>
#include <effect_codegen.hpp>
#include <effect_preprocessor.hpp>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

#include "config/config_globals.hpp"
#include "effect_module.hpp"
#include "vk/macros.hpp"

vkShade::Effect::Effect(VulkanDevice& device, VkExtent2D extent, VkFormat format, const std::string& fileName)
    : VulkanObject(device)
{
    // Create sampler for input texture
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };
    VK_CHECK(m_device.dispatch.CreateSampler(m_device.handle, &samplerInfo, nullptr, &m_sampler));

    // Get the ReShade shaders directory
    std::filesystem::path dataDir = DATADIR;
    dataDir = dataDir / "vkShade";
    std::filesystem::path effectPath = dataDir / "shaders" / fileName;

    // Compile the ReShade effect from source
    reshadefx::effect_module module;
    if (!this->compile(extent, effectPath))
        throw std::runtime_error("Failed to compile ReShade effect");

    this->create_descriptor_sets();
    this->create_pipeline(format);
    this->reflect_uniforms();
}

vkShade::Effect::~Effect()
{
    m_device.dispatch.DestroyPipeline(m_device.handle, m_pipeline, nullptr);
    m_device.dispatch.DestroyPipelineLayout(m_device.handle, m_pipelineLayout, nullptr);
    m_device.dispatch.DestroyDescriptorSetLayout(m_device.handle, m_uniformSetLayout, nullptr);
    m_device.dispatch.DestroyDescriptorSetLayout(m_device.handle, m_imageSetLayout, nullptr);
}

void vkShade::Effect::apply(VkCommandBuffer cmd, VkExtent2D extent)
{
    // Bind pipeline
    m_device.dispatch.CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    // Set viewport and scissor
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)extent.width,
        .height = (float)extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    m_device.dispatch.CmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = extent,
    };
    m_device.dispatch.CmdSetScissor(cmd, 0, 1, &scissor);

    // Bind descriptor sets
    std::array<VkDescriptorSet, 2> sets = { m_uniformSet, m_imageSet };
    m_device.dispatch.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);

    // Draw fullscreen triangle (3 vertices, no vertex buffer)
    m_device.dispatch.CmdDraw(cmd, 3, 1, 0, 0);
}

void vkShade::Effect::bind_input(VkImageView inputView)
{
    VkDescriptorImageInfo imageInfo = {
        .sampler = m_sampler,
        .imageView = inputView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_imageSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };

    m_device.dispatch.UpdateDescriptorSets(m_device.handle, 1, &descriptorWrite, 0, nullptr);
}

bool vkShade::Effect::compile(VkExtent2D extent, std::filesystem::path filePath)
{
    const bool useDebugInfo = false;
    const bool useSpecConstants = false;
    const bool enable16BitTypes = false;
    const bool invertYAxis = true;

    reshadefx::preprocessor pp;
    pp.add_macro_definition("__RESHADE__", std::to_string(60703));
    pp.add_macro_definition("__RESHADE_PERFORMANCE_MODE__", "0");

	pp.add_macro_definition("BUFFER_WIDTH", std::to_string(extent.width));
	pp.add_macro_definition("BUFFER_HEIGHT", std::to_string(extent.height));
	pp.add_macro_definition("BUFFER_RCP_WIDTH", "(1.0 / BUFFER_WIDTH)");
	pp.add_macro_definition("BUFFER_RCP_HEIGHT", "(1.0 / BUFFER_HEIGHT)");

    // TODO: Set from config
    pp.add_include_path("/opt/reshade/shaders");

	if (!pp.append_file(filePath))
	{
        // TODO: Improve this
        spdlog::error("ReShade FX compilation failed (append): {}", filePath.string());
        return false;
	}

    std::unique_ptr<reshadefx::codegen> backend;
    backend.reset(reshadefx::create_codegen_spirv(true, useDebugInfo, useSpecConstants, enable16BitTypes, invertYAxis));

	reshadefx::parser parser;
	if (!parser.parse(pp.output(), backend.get()))
	{
        // TODO: Improve this
        spdlog::error("ReShade FX compilation failed (parse): {}", pp.errors());
        spdlog::error("ReShade FX compilation failed (parse): {}", parser.errors());
        return false;
	}

    m_module = std::make_unique<reshadefx::effect_module>(backend->module());
    if (m_module->techniques.empty() || m_module->techniques[0].passes.empty())
    {
        spdlog::error("ReShade FX: no techniques found in {}", filePath.string());
        return false;
    }

    std::string vsEntryPoint = m_module->techniques[0].passes[0].vs_entry_point;
    std::string psEntryPoint = m_module->techniques[0].passes[0].ps_entry_point;

    std::string vs_binary, vs_asm, ps_binary, ps_asm, errors;

    if (!backend->assemble_code_for_entry_point(vsEntryPoint, vs_binary, vs_asm, errors))
    {
        spdlog::error("ReShade FX VS assembly failed: {}", errors);
        return false;
    }

    if (!backend->assemble_code_for_entry_point(psEntryPoint, ps_binary, ps_asm, errors))
    {
        spdlog::error("ReShade FX PS assembly failed: {}", errors);
        return false;
    }

    auto to_spirv = [](const std::string& binary)
    {
        std::vector<uint32_t> spirv(binary.size() / sizeof(uint32_t));
        std::memcpy(spirv.data(), binary.data(), binary.size());
        return spirv;
    };

    std::vector<uint32_t> vsBytecode = to_spirv(vs_binary);
    std::vector<uint32_t> psBytecode = to_spirv(ps_binary);

    spdlog::trace("VertexShader size (words): {}", vsBytecode.size());
    spdlog::trace("PixelShader size (words): {}", psBytecode.size());

    m_vertShader = std::make_shared<vkShade::ShaderModule>(m_device, vsBytecode);
    m_fragShader = std::make_shared<vkShade::ShaderModule>(m_device, psBytecode);

    return true;
}

void vkShade::Effect::create_descriptor_sets()
{
    auto& pass = m_module->techniques[0].passes[0];

    // Create Descriptor Pool
    std::vector<VkDescriptorPoolSize> poolSizes;
    if (m_module->total_uniform_size > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
    if (!pass.texture_bindings.empty())
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                              (uint32_t)pass.texture_bindings.size() });

    // Always at least 1 for the empty UBO set
    uint32_t maxSets = 1 + (!pass.texture_bindings.empty() ? 1 : 0);

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = (uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(m_device.dispatch.CreateDescriptorPool(m_device.handle, &poolInfo, nullptr, &m_descriptorPool));

    // Set 0: Uniform Buffer
    spdlog::trace("Uniform buffer size (bytes): {}", m_module->total_uniform_size);
    if (m_module->total_uniform_size > 0)
    {
        VkDescriptorSetLayoutBinding uboBinding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uboBinding,
        };

        VK_CHECK(m_device.dispatch.CreateDescriptorSetLayout(m_device.handle, &layoutInfo, nullptr, &m_uniformSetLayout));

        // Create the buffer
        m_uniformBuffer = std::make_unique<VulkanBuffer>(m_device, m_module->total_uniform_size,
                                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
    else
    {
        VkDescriptorSetLayoutCreateInfo emptyLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 0,
            .pBindings = nullptr,
        };

        VK_CHECK(m_device.dispatch.CreateDescriptorSetLayout(m_device.handle, &emptyLayoutInfo, nullptr, &m_uniformSetLayout));
    }

    // Allocate set 0
    VkDescriptorSetAllocateInfo uniformAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_uniformSetLayout,
    };

    VK_CHECK(m_device.dispatch.AllocateDescriptorSets(m_device.handle, &uniformAllocInfo, &m_uniformSet));

    // Write set 0 if we have a buffer
    if (m_uniformBuffer)
    {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = m_uniformBuffer->buffer(),
            .offset = 0,
            .range = m_module->total_uniform_size,
        };

        VkWriteDescriptorSet uboWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_uniformSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo,
        };

        m_device.dispatch.UpdateDescriptorSets(m_device.handle, 1, &uboWrite, 0, nullptr);
    }

    // Set 1: Texture Bindings
    if (!pass.texture_bindings.empty())
    {
        std::vector<VkDescriptorSetLayoutBinding> imageBindings;
        for (auto& tb : pass.texture_bindings)
        {
            imageBindings.push_back({
                .binding = tb.entry_point_binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            });
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = (uint32_t)imageBindings.size(),
            .pBindings = imageBindings.data(),
        };

        VK_CHECK(m_device.dispatch.CreateDescriptorSetLayout(m_device.handle, &layoutInfo, nullptr, &m_imageSetLayout));

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_imageSetLayout,
        };

        VK_CHECK(m_device.dispatch.AllocateDescriptorSets(m_device.handle, &allocInfo, &m_imageSet));
    }
}

void vkShade::Effect::create_pipeline(VkFormat outputFormat)
{
    auto& pass = m_module->techniques[0].passes[0];

    // Create pipeline layout
    std::array<VkDescriptorSetLayout, 2> layouts = { m_uniformSetLayout, m_imageSetLayout };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
    };

    VK_CHECK(m_device.dispatch.CreatePipelineLayout(m_device.handle, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    // Create graphics pipeline
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = m_vertShader->module(),
            .pName = pass.vs_entry_point.c_str(),
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = m_fragShader->module(),
            .pName = pass.ps_entry_point.c_str(),
        }
    };

    // No vertex input (fullscreen triangle generated in vertex shader)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    // Dynamic rendering info
    VkPipelineRenderingCreateInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &outputFormat,
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
    };

    VK_CHECK(m_device.dispatch.CreateGraphicsPipelines(m_device.handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));
    spdlog::debug("Effect pipeline created");
}

void vkShade::Effect::reflect_uniforms()
{
    auto reflect_type = [](const reshadefx::type type) -> vkShade::Uniform::Type
    {
        if (type.base == reshadefx::type::t_float && type.rows == 1 && type.cols == 1)
            return Uniform::Type::Float;
        if (type.base == reshadefx::type::t_float && type.rows == 2 && type.cols == 1)
            return Uniform::Type::Vec2;
        if (type.base == reshadefx::type::t_float && type.rows == 3 && type.cols == 1)
            return Uniform::Type::Vec3;
        if (type.base == reshadefx::type::t_float && type.rows == 4 && type.cols == 1)
            return Uniform::Type::Vec4;

        return Uniform::Type::Unknown;
    };

    // Convert the ReShade uniform to our own reflected type
    for (const auto& uniform : m_module->uniforms)
    {
        m_uniformsByName[uniform.name] = {
            .name = uniform.name,
            .size = uniform.size,
            .offset = uniform.offset,
            .type = reflect_type(uniform.type)
        };

        // Set the default value if there is one
        if (uniform.has_initializer_value)
        {
            // NOTE: The initializer value is a union, so we just cast it to a pointer.
            m_uniformBuffer->write(&uniform.initializer_value.as_uint[0], uniform.size, uniform.offset);
        }
    }
}
