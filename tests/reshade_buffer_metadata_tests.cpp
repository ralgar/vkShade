#include <catch2/catch_test_macros.hpp>

#include "runtime/reshade_buffer_metadata.hpp"

TEST_CASE("ReShade buffer bit depth follows the reference format semantics")
{
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_UNDEFINED) == 0);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_R5G6B5_UNORM_PACK16) == 5);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_R8G8B8A8_UNORM) == 8);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_A8B8G8R8_SRGB_PACK32) == 8);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32) == 9);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_A2B10G10R10_UNORM_PACK32) == 10);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_B10G11R11_UFLOAT_PACK32) == 11);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_R16G16B16A16_SFLOAT) == 16);
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_R32G32B32A32_SFLOAT) == 32);

    // ReShade does not classify the UNORM variant as a supported back-buffer
    // bit-depth format, despite its nominal 16 bits per component.
    CHECK(vkShade::get_reshade_buffer_color_bit_depth(VK_FORMAT_R16G16B16A16_UNORM) == 0);
}

TEST_CASE("ReShade buffer color format follows the reference API values")
{
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_UNDEFINED) == 0);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_R8G8B8A8_UNORM) == 28);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_A8B8G8R8_UNORM_PACK32) == 28);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_B8G8R8A8_UNORM) == 87);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_A2B10G10R10_UNORM_PACK32) == 24);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_A2R10G10B10_UNORM_PACK32)
          == 0x42475331);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_R16G16B16A16_SFLOAT) == 10);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_R32G32B32_SFLOAT) == 6);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_R32G32B32A32_SFLOAT) == 2);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32) == 67);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_B10G11R11_UFLOAT_PACK32) == 26);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_R5G6B5_UNORM_PACK16) == 85);
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_A1R5G5B5_UNORM_PACK16) == 86);
}

TEST_CASE("ReShade buffer color format normalizes sRGB image formats")
{
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_R8G8B8A8_SRGB)
          == vkShade::get_reshade_buffer_color_format(VK_FORMAT_R8G8B8A8_UNORM));
    CHECK(vkShade::get_reshade_buffer_color_format(VK_FORMAT_B8G8R8A8_SRGB)
          == vkShade::get_reshade_buffer_color_format(VK_FORMAT_B8G8R8A8_UNORM));
}
