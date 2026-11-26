#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace vkShade
{
    uint32_t get_reshade_buffer_color_bit_depth(VkFormat format);
    uint32_t get_reshade_buffer_color_format(VkFormat format);
} // namespace vkShade
