#include "effect.hpp"

#include <filesystem>

#include <magic_enum/magic_enum.hpp>

#include "core/resource_cache.hpp"
#include "core/service_locator.hpp"

vkShade::Effect::Effect(VulkanDevice& device, VkFormat outputFormat, const std::string& fileName)
    : VulkanObject(device)
{
    auto& shaderCache = vkShade::Locator<vkShade::ResourceCache<vkShade::ShaderModule>>::get();

    const char* homeDir = std::getenv("HOME");
    if (!homeDir)
        spdlog::critical("HOME environment variable not set!");
    std::filesystem::path dataDir = std::filesystem::path(homeDir) / ".local/share/vkShade";

    // Load the vertex shader module
    std::filesystem::path vertShaderPath = dataDir / "shaders" / "fullscreen.vert.spv";
    m_vertShader = shaderCache.load(vertShaderPath, m_device, vertShaderPath);
	if (m_vertShader == nullptr)
        spdlog::error("Vertex shader not found");

    // Load the fragment shader module
    std::filesystem::path fragShaderPath = dataDir / "shaders" / fileName;
    m_fragShader = shaderCache.load(fragShaderPath, m_device, fragShaderPath);
	if (m_fragShader == nullptr)
        spdlog::error("Fragment shader not found");

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
    m_device.dispatch.CreateSampler(m_device.handle, &samplerInfo, nullptr, &m_sampler);

    // Create descriptor set layout (for sampling input texture)
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding,
    };

    m_device.dispatch.CreateDescriptorSetLayout(m_device.handle, &layoutInfo, nullptr, &m_descriptorSetLayout);

    create_descriptor_pool();
    allocate_descriptor_set();

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout,
    };

    m_device.dispatch.CreatePipelineLayout(m_device.handle, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

    // Create graphics pipeline
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = m_vertShader->module(),
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = m_fragShader->module(),
            .pName = "main",
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

    VkResult result = m_device.dispatch.CreateGraphicsPipelines(m_device.handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to create pipeline: {}", magic_enum::enum_name(result));
    }

    // In Effect constructor after CreateGraphicsPipelines:
    if (m_pipeline == VK_NULL_HANDLE)
    {
        spdlog::error("Failed to create pipeline!");
    }

    spdlog::debug("Effect pipeline created");
}

vkShade::Effect::~Effect()
{
    m_device.dispatch.DestroyPipeline(m_device.handle, m_pipeline, nullptr);
    m_device.dispatch.DestroyPipelineLayout(m_device.handle, m_pipelineLayout, nullptr);
    m_device.dispatch.DestroyDescriptorSetLayout(m_device.handle, m_descriptorSetLayout, nullptr);
}

void vkShade::Effect::allocate_descriptor_set()
{
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_descriptorSetLayout,
    };

    m_device.dispatch.AllocateDescriptorSets(m_device.handle, &allocInfo, &m_descriptorSet);
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

    // Bind descriptor set
    m_device.dispatch.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

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
        .dstSet = m_descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };

    m_device.dispatch.UpdateDescriptorSets(m_device.handle, 1, &descriptorWrite, 0, nullptr);
}

void vkShade::Effect::create_descriptor_pool()
{
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };

    m_device.dispatch.CreateDescriptorPool(m_device.handle, &poolInfo, nullptr, &m_descriptorPool);
}
