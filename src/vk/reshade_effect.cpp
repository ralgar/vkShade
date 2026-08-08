#include "reshade_effect.hpp"

#include <cstring>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <effect_parser.hpp>
#include <effect_codegen.hpp>
#include <effect_module.hpp>
#include <effect_preprocessor.hpp>
#include <magic_enum/magic_enum.hpp>
#include "core/logger.hpp"
#include <vulkan/vulkan_core.h>

#include "core/service_locator.hpp"
#include "config/config_manager.hpp"
#include "vk/image.hpp"
#include "vk/initializers.hpp"
#include "vk/macros.hpp"
#include "vk/reshade_uniforms.hpp"
#include "vk/sampler.hpp"

namespace
{
    uint32_t format_bit_depth(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_B5G6R5_UNORM_PACK16:
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
                return 5;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return 8;
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                return 9;
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
                return 10;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return 11;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 16;
            case VK_FORMAT_R32G32B32_SFLOAT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 32;
            default:
                return 0;
        }
    }
}

vkShade::ReshadeEffect::ReshadeEffect(VulkanDevice& device, VkExtent2D extent, VkFormat format, std::filesystem::path effectPath)
    : VulkanObject(device), m_extent(extent), m_format(format), m_fileName(effectPath.filename())
{
    Logger::trace("Creating effect: {}", effectPath.filename().string());

    // Compile the ReShade effect from source
    reshadefx::effect_module module;
    if (!this->compile(extent, effectPath))
        throw std::runtime_error("Failed to load effect: " + effectPath.filename().string());

    this->reflect_images();
    this->reflect_samplers();
    this->reflect_descriptors();
    this->reflect_pipeline();
    this->reflect_uniforms();

    Logger::debug("Created effect: {}", effectPath.filename().string());
}

vkShade::ReshadeEffect::~ReshadeEffect()
{
    for (const auto& [name, uniform] : m_uniformsByName)
    {
        auto& preset = vkShade::Locator<vkShade::ConfigManager>::get().preset();
        Uniform::dispatch_type(uniform.type, [&]<typename T>(std::type_identity<T>)
        {
            preset.on_changed(m_fileName, name).disconnect<&ReshadeEffect::on_uniform_changed<T>>(this);
        });
    }

    for (const auto& pass : m_passes)
    {
        m_device.dispatch.DestroyPipeline(m_device.handle, pass.pipeline, nullptr);
        m_device.dispatch.DestroyPipelineLayout(m_device.handle, pass.pipelineLayout, nullptr);
        m_device.dispatch.DestroyDescriptorSetLayout(m_device.handle, pass.imageSetLayout, nullptr);
    }

    m_device.dispatch.DestroyDescriptorSetLayout(m_device.handle, m_uniformSetLayout, nullptr);
    m_device.dispatch.DestroyDescriptorPool(m_device.handle, m_descriptorPool, nullptr);
}

void vkShade::ReshadeEffect::apply(VkCommandBuffer cmd, VulkanImage& outputImage)
{
    auto& technique = m_module->techniques[0];

    // ReShade defines newly created effect textures to contain zero. Initialize
    // all internal resources before any pass can sample temporal history or
    // load an attachment that has not been written yet.
    if (!m_imagesInitialized)
    {
        for (auto& [name, texture] : m_textures)
            texture->initialize(cmd);
        m_imagesInitialized = true;
    }

    // Clear the stencil buffer if we have one.
    if (m_stencilBuffer)
    {
        m_stencilBuffer->transition_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkClearDepthStencilValue clearValue = { .depth = 0.0f, .stencil = 0 };
        VkImageSubresourceRange range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_STENCIL_BIT);

        m_device.dispatch.CmdClearDepthStencilImage(cmd, m_stencilBuffer->image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);
    }

    // Iterate and apply effect passes
    for (auto&& [pass, passInfo] : std::views::zip(m_passes, technique.passes))
    {
        const VkClearValue clearValue = { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } };
        const VkClearValue* clear = passInfo.clear_render_targets ? &clearValue : nullptr;

        VkExtent2D attachmentExtent {
            .width = outputImage.extent().width,
            .height = outputImage.extent().height,
        };
        std::vector<VkRenderingAttachmentInfo> colorAttachments;
        for (const auto& name : passInfo.render_target_names)
        {
            if (name.empty())
                break;  // render_target_names are contiguous, empty means end

            auto* renderTarget = m_textures.at(name).get();
            if (colorAttachments.empty())
            {
                attachmentExtent.width = renderTarget->extent().width;
                attachmentExtent.height = renderTarget->extent().height;
            }
            renderTarget->transition_render_target_layout(
                cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            colorAttachments.push_back(vkinit::rendering_attachment_info(
                renderTarget->render_target_view(passInfo.srgb_write_enable), clear,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
        }

        if (colorAttachments.empty())
        {
            outputImage.transition_layout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            colorAttachments.push_back(vkinit::rendering_attachment_info(
                outputImage.render_target_view(passInfo.srgb_write_enable), clear,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
        }

        VkExtent2D passExtent {
            .width = passInfo.viewport_width ? passInfo.viewport_width : attachmentExtent.width,
            .height = passInfo.viewport_height ? passInfo.viewport_height : attachmentExtent.height,
        };

        // Set up stencil attachment if there is one
        VkRenderingAttachmentInfo* stencilAttachment = nullptr;
        VkRenderingAttachmentInfo stencilAttachmentInfo;

        if (passInfo.stencil_enable && m_stencilBuffer
            && passExtent.width == m_stencilBuffer->extent().width
            && passExtent.height == m_stencilBuffer->extent().height)
        {
            m_stencilBuffer->transition_layout(cmd, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);

            stencilAttachmentInfo = vkinit::rendering_attachment_info(
                m_stencilBuffer->image_view(),
                nullptr,
                VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL);

            stencilAttachment = &stencilAttachmentInfo;
        }

        // Begin render pass
        VkRenderingInfo renderingInfo = vkinit::rendering_info(passExtent, colorAttachments,
                                                               nullptr, stencilAttachment);
        m_device.dispatch.CmdBeginRendering(cmd, &renderingInfo);

        // Bind pipeline
        m_device.dispatch.CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass.pipeline);

        // Set viewport and scissor
        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(passExtent.width),
            .height = static_cast<float>(passExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        m_device.dispatch.CmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = passExtent,
        };
        m_device.dispatch.CmdSetScissor(cmd, 0, 1, &scissor);

        // Bind descriptor sets
        std::array<VkDescriptorSet, 2> sets = { m_uniformSet, pass.imageSet };
        m_device.dispatch.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pass.pipelineLayout, 0, sets.size(), sets.data(), 0, nullptr);

        if (passInfo.stencil_enable)
        {
            m_device.dispatch.CmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK,
                passInfo.stencil_reference_value);
        }

        // Draw n vertices as defined in the pass (no vertex buffer)
        m_device.dispatch.CmdDraw(cmd, passInfo.num_vertices, 1, 0, 0);

        m_device.dispatch.CmdEndRendering(cmd);

        // Make internal render targets visible to subsequent passes and frames.
        // ReShade regenerates their mip chain after every writing pass unless
        // GenerateMipmaps is disabled on that pass.
        for (const auto& name : passInfo.render_target_names)
        {
            if (name.empty())
                break;

            auto* renderTarget = m_textures.at(name).get();
            renderTarget->transition_render_target_layout(
                cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            if (passInfo.generate_mipmaps)
                renderTarget->generate_mipmaps(cmd);
        }
    }
}

void vkShade::ReshadeEffect::bind_input(VulkanImage& colorImage, VulkanImage& depthImage)
{
    for (auto&& [pass, passInfo] : std::views::zip(m_passes, m_module->techniques[0].passes))
    {
        for (const auto& binding : passInfo.texture_bindings)
        {
            auto& samplerInfo = m_module->samplers.at(binding.index);
            auto& sampler = m_samplers.at(binding.index);

            auto texIt = std::find_if(m_module->textures.begin(), m_module->textures.end(),
                [&](const auto& t) { return t.unique_name == samplerInfo.texture_name; });

            if (texIt == m_module->textures.end())
                continue;

            VkImageView imageView;
            if (texIt->semantic == "COLOR")
                imageView = colorImage.sampled_view(binding.srgb);
            else if (texIt->semantic == "DEPTH")
                imageView = depthImage.sampled_view(binding.srgb);
            else
                continue;  // Skip other textures (we only bind COLOR and DEPTH here)

            VkDescriptorImageInfo imageInfo = {
                .sampler = sampler->handle(),
                .imageView = imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkWriteDescriptorSet write = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = pass.imageSet,
                .dstBinding = binding.entry_point_binding,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            m_device.dispatch.UpdateDescriptorSets(m_device.handle, 1, &write, 0, nullptr);
        }
    }
}

bool vkShade::ReshadeEffect::compile(VkExtent2D extent, std::filesystem::path filePath)
{
    const bool useDebugInfo = false;
    const bool useSpecConstants = false;
    const bool enable16BitTypes = false;
    const bool invertYAxis = true;

    reshadefx::preprocessor pp;
    pp.add_macro_definition("__RESHADE__", std::to_string(60703));
    pp.add_macro_definition("__RESHADE_PERFORMANCE_MODE__", "0");
    pp.add_macro_definition("__RENDERER__", "0x21300");

	pp.add_macro_definition("BUFFER_WIDTH", std::to_string(extent.width));
	pp.add_macro_definition("BUFFER_HEIGHT", std::to_string(extent.height));
	pp.add_macro_definition("BUFFER_RCP_WIDTH", "(1.0 / BUFFER_WIDTH)");
	pp.add_macro_definition("BUFFER_RCP_HEIGHT", "(1.0 / BUFFER_HEIGHT)");
	pp.add_macro_definition("BUFFER_COLOR_BIT_DEPTH", std::to_string(format_bit_depth(m_format)));

    // Add include paths
    auto& config = vkShade::Locator<vkShade::ConfigManager>::get().app();
    auto effectPaths = config.get<std::vector<std::string>>("ReShade", "EffectSearchPaths");
    for (auto& path : effectPaths.value_or(std::vector<std::string>{}))
        pp.add_include_path(path);

    // Add some conversion macros for compatibility with older versions of ReShade
    pp.append_string(
        "#define tex2Doffset(s, coords, offset) tex2D(s, coords, offset)\n"
        "#define tex2Dlodoffset(s, coords, offset) tex2Dlod(s, coords, offset)\n"
        "#define tex2Dgather(s, t, c) tex2Dgather##c(s, t)\n"
        "#define tex2Dgatheroffset(s, t, o, c) tex2Dgather##c(s, t, o)\n"
        "#define tex2Dgather0 tex2DgatherR\n"
        "#define tex2Dgather1 tex2DgatherG\n"
        "#define tex2Dgather2 tex2DgatherB\n"
        "#define tex2Dgather3 tex2DgatherA\n");

    if (!pp.append_file(filePath))
	{
        Logger::error(pp.errors());
        return false;
	}

    std::unique_ptr<reshadefx::codegen> backend;
    backend.reset(reshadefx::create_codegen_spirv(true, useDebugInfo, useSpecConstants, enable16BitTypes, invertYAxis));

	reshadefx::parser parser;
	if (!parser.parse(pp.output(), backend.get()))
	{
        Logger::error(parser.errors());
        return false;
	}

    m_module = std::make_unique<reshadefx::effect_module>(backend->module());
    if (m_module->techniques.empty() || m_module->techniques[0].passes.empty())
    {
        Logger::error("No techniques found: {}", filePath.string());
        return false;
    }

    for (const auto& pass : m_module->techniques[0].passes)
    {
        std::string vsEntryPoint = pass.vs_entry_point;
        std::string psEntryPoint = pass.ps_entry_point;

        std::string vs_binary, vs_asm, ps_binary, ps_asm, errors;

        if (!backend->assemble_code_for_entry_point(vsEntryPoint, vs_binary, vs_asm, errors))
        {
            Logger::error("Failed to assemble vertex shader: {}", errors);
            return false;
        }

        if (!backend->assemble_code_for_entry_point(psEntryPoint, ps_binary, ps_asm, errors))
        {
            Logger::error("Failed to assemble pixel shader: {}", errors);
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

        m_passes.emplace_back(Pass{
            .vertexShader = std::make_shared<vkShade::ShaderModule>(m_device, vsBytecode),
            .fragmentShader = std::make_shared<vkShade::ShaderModule>(m_device, psBytecode)
        });
    }

    return true;
}

VkBlendFactor vkShade::ReshadeEffect::convert_blend_factor(reshadefx::blend_factor blendFactor)
{
    switch (blendFactor)
    {
        case reshadefx::blend_factor::zero:                     return VK_BLEND_FACTOR_ZERO;
        case reshadefx::blend_factor::one:                      return VK_BLEND_FACTOR_ONE;
        case reshadefx::blend_factor::source_color:             return VK_BLEND_FACTOR_SRC_COLOR;
        case reshadefx::blend_factor::one_minus_source_color:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case reshadefx::blend_factor::dest_color:               return VK_BLEND_FACTOR_DST_COLOR;
        case reshadefx::blend_factor::one_minus_dest_color:     return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case reshadefx::blend_factor::source_alpha:             return VK_BLEND_FACTOR_SRC_ALPHA;
        case reshadefx::blend_factor::one_minus_source_alpha:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case reshadefx::blend_factor::dest_alpha:               return VK_BLEND_FACTOR_DST_ALPHA;
        case reshadefx::blend_factor::one_minus_dest_alpha:     return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    }

    std::unreachable();
}

VkBlendOp vkShade::ReshadeEffect::convert_blend_op(reshadefx::blend_op blendOp)
{
    switch (blendOp)
    {
        case reshadefx::blend_op::add:              return VK_BLEND_OP_ADD;
        case reshadefx::blend_op::subtract:         return VK_BLEND_OP_SUBTRACT;
        case reshadefx::blend_op::reverse_subtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case reshadefx::blend_op::min:              return VK_BLEND_OP_MIN;
        case reshadefx::blend_op::max:              return VK_BLEND_OP_MAX;
    }

    std::unreachable();
}

VkPrimitiveTopology vkShade::ReshadeEffect::convert_primitive_topology(reshadefx::primitive_topology topology)
{
    switch (topology)
    {
        case reshadefx::primitive_topology::point_list:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case reshadefx::primitive_topology::line_list:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case reshadefx::primitive_topology::line_strip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case reshadefx::primitive_topology::triangle_list:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case reshadefx::primitive_topology::triangle_strip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }

    std::unreachable();
}

VkCompareOp vkShade::ReshadeEffect::convert_stencil_func(reshadefx::stencil_func stencilFunc)
{
    switch (stencilFunc)
    {
        case reshadefx::stencil_func::never:         return VK_COMPARE_OP_NEVER;
        case reshadefx::stencil_func::less:          return VK_COMPARE_OP_LESS;
        case reshadefx::stencil_func::equal:         return VK_COMPARE_OP_EQUAL;
        case reshadefx::stencil_func::less_equal:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case reshadefx::stencil_func::greater:       return VK_COMPARE_OP_GREATER;
        case reshadefx::stencil_func::not_equal:     return VK_COMPARE_OP_NOT_EQUAL;
        case reshadefx::stencil_func::greater_equal: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case reshadefx::stencil_func::always:        return VK_COMPARE_OP_ALWAYS;
    }

    std::unreachable();
}

VkStencilOp vkShade::ReshadeEffect::convert_stencil_op(reshadefx::stencil_op stencilOp)
{
    switch (stencilOp)
    {
        case reshadefx::stencil_op::zero:               return VK_STENCIL_OP_ZERO;
        case reshadefx::stencil_op::keep:               return VK_STENCIL_OP_KEEP;
        case reshadefx::stencil_op::replace:            return VK_STENCIL_OP_REPLACE;
        case reshadefx::stencil_op::increment_saturate: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case reshadefx::stencil_op::decrement_saturate: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case reshadefx::stencil_op::invert:             return VK_STENCIL_OP_INVERT;
        case reshadefx::stencil_op::increment:          return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case reshadefx::stencil_op::decrement:          return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }

    std::unreachable();
}

vkShade::Uniform::Type vkShade::ReshadeEffect::convert_uniform_type(reshadefx::type type)
{
    using T = reshadefx::type;

    // Float
    if (type == T{ T::t_float, 1, 1 })  return Uniform::Type::Float;
    if (type == T{ T::t_float, 2, 1 })  return Uniform::Type::Float2;
    if (type == T{ T::t_float, 3, 1 })  return Uniform::Type::Float3;
    if (type == T{ T::t_float, 4, 1 })  return Uniform::Type::Float4;

    // Int
    if (type == T{ T::t_int, 1, 1 })    return Uniform::Type::Int;
    if (type == T{ T::t_int, 2, 1 })    return Uniform::Type::Int2;
    if (type == T{ T::t_int, 3, 1 })    return Uniform::Type::Int3;
    if (type == T{ T::t_int, 4, 1 })    return Uniform::Type::Int4;

    // Uint
    if (type == T{ T::t_uint, 1, 1 })   return Uniform::Type::Uint;
    if (type == T{ T::t_uint, 2, 1 })   return Uniform::Type::Uint2;
    if (type == T{ T::t_uint, 3, 1 })   return Uniform::Type::Uint3;
    if (type == T{ T::t_uint, 4, 1 })   return Uniform::Type::Uint4;

    // Bool
    if (type == T{ T::t_bool, 1, 1 })   return Uniform::Type::Bool;
    if (type == T{ T::t_bool, 2, 1 })   return Uniform::Type::Bool2;
    if (type == T{ T::t_bool, 3, 1 })   return Uniform::Type::Bool3;
    if (type == T{ T::t_bool, 4, 1 })   return Uniform::Type::Bool4;

    throw std::invalid_argument("Unsupported uniform type");;
}

void vkShade::ReshadeEffect::reflect_descriptors()
{
    auto& technique = m_module->techniques[0];

    // Create the descriptor pool
    uint32_t totalImageBindings = 0;
    for (const auto& pass : technique.passes)
        totalImageBindings += pass.texture_bindings.size();

    std::vector<VkDescriptorPoolSize> poolSizes;
    if (m_module->total_uniform_size > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });
    if (totalImageBindings > 0)
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalImageBindings });

    // One UBO set, and an image set per-pass.
    uint32_t maxSets = 1 + technique.passes.size();

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = (uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VK_CHECK(m_device.dispatch.CreateDescriptorPool(m_device.handle, &poolInfo, nullptr, &m_descriptorPool));

    // Set 0: Uniform Buffer
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

    // Per-pass texture sets
    for (auto&& [pass, passInfo] : std::views::zip(m_passes, m_module->techniques[0].passes))
    {
        // Allocate the set
        std::vector<VkDescriptorSetLayoutBinding> textureBindings;
        for (auto& binding : passInfo.texture_bindings)
        {
            textureBindings.push_back({
                .binding = binding.entry_point_binding,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            });
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = (uint32_t)textureBindings.size(),
            .pBindings = textureBindings.data(),
        };

        VK_CHECK(m_device.dispatch.CreateDescriptorSetLayout(m_device.handle, &layoutInfo, nullptr, &pass.imageSetLayout));

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &pass.imageSetLayout,
        };

        VK_CHECK(m_device.dispatch.AllocateDescriptorSets(m_device.handle, &allocInfo, &pass.imageSet));

        // Write the set, unless its a semantic (COLOR or DEPTH) image.
        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(passInfo.texture_bindings.size());
        std::vector<VkWriteDescriptorSet> writes;

        for (const auto& binding : passInfo.texture_bindings)
        {
            auto& samplerInfo = m_module->samplers.at(binding.index);
            auto& sampler = m_samplers.at(binding.index);

            auto texIt = std::find_if(m_module->textures.begin(), m_module->textures.end(),
                [&](const auto& t) { return t.unique_name == samplerInfo.texture_name; });

            if (texIt != m_module->textures.end() && !texIt->semantic.empty())
                continue;  // Skip semantic textures, they're bound per-frame

            auto& texture = m_textures.at(samplerInfo.texture_name);
            imageInfos.push_back({
                .sampler = sampler->handle(),
                .imageView = texture->sampled_view(binding.srgb),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = pass.imageSet,
                .dstBinding = binding.entry_point_binding,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfos.back(),
            });
        }

        if (!writes.empty())
            m_device.dispatch.UpdateDescriptorSets(m_device.handle, writes.size(), writes.data(), 0, nullptr);
    }
}

void vkShade::ReshadeEffect::reflect_images()
{
    // Iterate and check for depth textures since we don't support them yet
    bool warnedAboutDepth = false;
    for (const auto& pass : m_module->techniques[0].passes)
    {
        for (const auto& binding : pass.texture_bindings)
        {
            auto& textureName = m_module->samplers.at(binding.index).texture_name;
            auto texIt = std::find_if(m_module->textures.begin(), m_module->textures.end(),
                [&](const auto& t) { return t.unique_name == textureName; });

            // Warn about lack of proper depth support
            if (!warnedAboutDepth && texIt != m_module->textures.end() && texIt->semantic == "DEPTH")
            {
                Logger::warn("Effect may require depth buffer access, which is not yet supported.");
                warnedAboutDepth = true;
            }
        }
    }

    // Create/load the textures
    for (const auto& info : m_module->textures)
    {
        if (!info.semantic.empty())
            continue;  // We don't own these, the application does.

        m_textures[info.unique_name] = std::make_unique<vkShade::VulkanImage>(m_device, info);
    }

    // Allocate stencil buffer if any pass requires it
    bool hasStencil = std::any_of(m_module->techniques[0].passes.begin(),
                                  m_module->techniques[0].passes.end(),
                                  [](const auto& p) { return p.stencil_enable; });

    if (hasStencil)
    {
        VkImageUsageFlags usage {};
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        m_stencilBuffer = std::make_unique<VulkanImage>(m_device, m_extent, VK_FORMAT_S8_UINT, usage);
    }
}

void vkShade::ReshadeEffect::reflect_pipeline()
{
    for (auto&& [pass, passInfo] : std::views::zip(m_passes, m_module->techniques[0].passes))
    {
        std::vector<VkFormat> colorFormats;
        for (const auto& name : passInfo.render_target_names)
        {
            if (name.empty())
                break;

            auto it = m_textures.find(name);
            if (it != m_textures.end())
                colorFormats.push_back(VulkanImage::view_format(it->second->format(),
                                                                passInfo.srgb_write_enable));
        }

        if (colorFormats.empty())
            colorFormats.push_back(VulkanImage::view_format(m_format,
                                                            passInfo.srgb_write_enable));

        // Create pipeline layout
        std::array<VkDescriptorSetLayout, 2> layouts = { m_uniformSetLayout, pass.imageSetLayout };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = layouts.size(),
            .pSetLayouts = layouts.data(),
        };

        VK_CHECK(m_device.dispatch.CreatePipelineLayout(m_device.handle, &pipelineLayoutInfo, nullptr, &pass.pipelineLayout));

        // Create graphics pipeline
        VkPipelineShaderStageCreateInfo shaderStages[] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = pass.vertexShader->module(),
                .pName = passInfo.vs_entry_point.c_str(),
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = pass.fragmentShader->module(),
                .pName = passInfo.ps_entry_point.c_str(),
            }
        };

        // No vertex input (fullscreen triangle generated in vertex shader)
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = convert_primitive_topology(passInfo.topology),
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

        VkStencilOpState stencilOpState = {
            .failOp      = convert_stencil_op(passInfo.stencil_fail_op),
            .passOp      = convert_stencil_op(passInfo.stencil_pass_op),
            .depthFailOp = convert_stencil_op(passInfo.stencil_depth_fail_op),
            .compareOp   = convert_stencil_func(passInfo.stencil_comparison_func),
            .compareMask = static_cast<uint32_t>(passInfo.stencil_read_mask),
            .writeMask   = static_cast<uint32_t>(passInfo.stencil_write_mask),
            .reference   = static_cast<uint32_t>(passInfo.stencil_reference_value),
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .stencilTestEnable = passInfo.stencil_enable,
            .front             = stencilOpState,
            .back              = stencilOpState,
        };

        std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
        for (size_t i = 0; i < colorFormats.size(); i++)
        {
            blendAttachments.push_back({
                .blendEnable         = passInfo.blend_enable[i],
                .srcColorBlendFactor = convert_blend_factor(passInfo.source_color_blend_factor[i]),
                .dstColorBlendFactor = convert_blend_factor(passInfo.dest_color_blend_factor[i]),
                .colorBlendOp        = convert_blend_op(passInfo.color_blend_op[i]),
                .srcAlphaBlendFactor = convert_blend_factor(passInfo.source_alpha_blend_factor[i]),
                .dstAlphaBlendFactor = convert_blend_factor(passInfo.dest_alpha_blend_factor[i]),
                .alphaBlendOp        = convert_blend_op(passInfo.alpha_blend_op[i]),
                .colorWriteMask      = static_cast<VkColorComponentFlags>(passInfo.render_target_write_mask[i]),  // Yes, this works.
            });
        }
        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
            .pAttachments = blendAttachments.data(),
        };

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE
        };

        VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 3,
            .pDynamicStates = dynamicStates,
        };

        // Dynamic rendering info
        VkPipelineRenderingCreateInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = (uint32_t)colorFormats.size(),
            .pColorAttachmentFormats = colorFormats.data(),
            .stencilAttachmentFormat = passInfo.stencil_enable ? VK_FORMAT_S8_UINT : VK_FORMAT_UNDEFINED,
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
            .pDepthStencilState = &depthStencilInfo,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pass.pipelineLayout,
        };

        VK_CHECK(m_device.dispatch.CreateGraphicsPipelines(m_device.handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pass.pipeline));
    }
}

void vkShade::ReshadeEffect::reflect_samplers()
{
    for (const auto& samplerInfo : m_module->samplers)
    {
        m_samplers.push_back(std::make_unique<vkShade::VulkanSampler>(m_device, samplerInfo));
    }
}

void vkShade::ReshadeEffect::reflect_uniforms()
{
    // Convert the ReShade uniform to our own reflected type
    for (const auto& uniform : m_module->uniforms)
    {
        auto sourceIt = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
        {
            return a.name == "source";
        });

        if (sourceIt != uniform.annotations.end())
        {
            const auto& source = sourceIt->value.string_data;

            if (reshade_uniform_uses_initializer(source))
                sourceIt = uniform.annotations.end();
            else
            {
                // ReShade clears all source uniforms, including unknown sources,
                // rather than applying their shader initializer.
                std::vector<std::byte> zero(uniform.size);
                m_uniformBuffer->write(zero.data(), zero.size(), uniform.offset);

                if (source == "frametime")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::FrameTimeUniform>(uniform));
                else if (source == "framecount")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::FrameCountUniform>(uniform));
                else if (source == "date")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::DateUniform>(uniform));
                else if (source == "timer")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::TimerUniform>(uniform));
                else if (source == "pingpong")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::PingPongUniform>(uniform));
                else if (source == "random")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::RandomUniform>(uniform));
                else if (source == "key")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::KeyUniform>(uniform));
                else if (source == "mousebutton")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::MouseButtonUniform>(uniform));
                else if (source == "mousepoint")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::MousePointUniform>(uniform));
                else if (source == "mousedelta")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::MouseDeltaUniform>(uniform));
                else if (source == "mousewheel")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::MouseWheelUniform>(uniform));
                else if (source == "bufready_depth")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::DepthUniform>(uniform));
                else if (source == "overlay_open")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::OverlayOpenUniform>(uniform));
                else if (source == "overlay_active")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::OverlayActiveUniform>(uniform));
                else if (source == "overlay_hovered")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::OverlayHoveredUniform>(uniform));
                else if (source == "screenshot")
                    m_builtinUniforms.push_back(std::make_unique<vkShade::ScreenshotUniform>(uniform));

                continue;  // Skip GUI reflection for source uniforms
            }
        }

        // If we've reached here then it's a generic/GUI uniform

        // Store uniform data
        m_uniformsByName[uniform.name] = {
            .name = uniform.name,
            .size = uniform.size,
            .offset = uniform.offset,
            .type = convert_uniform_type(uniform.type)
        };

        // Subscribe to changes and set the initial value.
        auto& preset = vkShade::Locator<vkShade::ConfigManager>::get().preset();
        Uniform::dispatch_type(convert_uniform_type(uniform.type), [&]<typename T>(std::type_identity<T>)
        {
            // Write the uniform value. Either from the preset, the initializer, or by zero-filling.
            if (auto result = preset.get<T>(m_fileName, uniform.name))
            {
                std::string stringValue = preset.get<std::string>(m_fileName, uniform.name).value_or("");
                Logger::trace("[{}] Loading uniform '{}' from preset: {}", m_fileName, uniform.name, stringValue);

                m_uniformBuffer->write(&result.value(), uniform.size, uniform.offset);
            }
            else if (uniform.has_initializer_value)
            {
                Logger::trace("[{}] Initializing uniform '{}' from shader initializer", m_fileName, uniform.name);

                T value;

                // NOTE: The initializer value is a union, so we just cast it to a pointer.
                std::memcpy(&value, &uniform.initializer_value.as_uint[0], sizeof(T));

                m_uniformBuffer->write(&value, uniform.size, uniform.offset);
                preset.set(m_fileName, uniform.name, value);
            }
            else
            {
                Logger::trace("[{}] Initializing uniform '{}' to zero", m_fileName, uniform.name);
                std::vector<std::byte> zero(uniform.size);
                m_uniformBuffer->write(zero.data(), zero.size(), uniform.offset);
            }

            // Subscribe to uniform changes
            preset.on_changed(m_fileName, uniform.name).connect<&ReshadeEffect::on_uniform_changed<T>>(this);
        });
    }
}

void vkShade::ReshadeEffect::update(const ReshadeFrameState& frame)
{
    for (auto& uniform : m_builtinUniforms)
    {
        uniform->update(*m_uniformBuffer, frame);
    }
}
