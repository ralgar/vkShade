#include "reshade_buffer_metadata.hpp"

uint32_t vkShade::get_reshade_buffer_color_bit_depth(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            return 5;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
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

uint32_t vkShade::get_reshade_buffer_color_format(VkFormat format)
{
    // ReShade exposes values from api::format rather than backend-native format
    // enums and normalizes sRGB variants to their default linear typed format.
    switch (format)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            return 28;  // api::format::r8g8b8a8_unorm
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return 87;  // api::format::b8g8r8a8_unorm
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return 24;  // api::format::r10g10b10a2_unorm
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return 0x42475331;  // api::format::b10g10r10a2_unorm
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 10;  // api::format::r16g16b16a16_float
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 6;  // api::format::r32g32b32_float
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 2;  // api::format::r32g32b32a32_float
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            return 67;  // api::format::r9g9b9e5
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return 26;  // api::format::r11g11b10_float
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            return 85;  // api::format::b5g6r5_unorm
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            return 86;  // api::format::b5g5r5a1_unorm
        default:
            return 0;  // api::format::unknown
    }
}
