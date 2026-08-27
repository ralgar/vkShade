#include "reshade_uniforms.hpp"

#include <chrono>

#include <effect_module.hpp>
#include <glm/vec2.hpp>

#include "vk/buffer.hpp"

static bool uniform_has_source(const reshadefx::uniform& uniform, const std::string& source)
{
    auto it = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    return it != uniform.annotations.end() && it->value.string_data == source;
}

vkShade::FrameTimeUniform::FrameTimeUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "frametime"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::FrameTimeUniform::update(VulkanBuffer& buffer, const ReshadeFrameState& frame)
{
    buffer.write(&frame.frameTime, m_size, m_offset);
}

vkShade::FrameCountUniform::FrameCountUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "framecount"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::FrameCountUniform::update(VulkanBuffer& buffer, const ReshadeFrameState& frame)
{
    buffer.write(&frame.frameCount, m_size, m_offset);
}

vkShade::DateUniform::DateUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "date"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::DateUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    auto        now         = std::chrono::system_clock::now();
    std::time_t nowC        = std::chrono::system_clock::to_time_t(now);
    struct tm*  currentTime = std::localtime(&nowC);
    float       year        = 1900.0f + static_cast<float>(currentTime->tm_year);
    float       month       = 1.0f + static_cast<float>(currentTime->tm_mon);
    float       day         = static_cast<float>(currentTime->tm_mday);
    float       seconds     = static_cast<float>((currentTime->tm_hour * 60 + currentTime->tm_min) * 60 + currentTime->tm_sec);
    float       date[]      = {year, month, day, seconds};

    buffer.write(date, m_size, m_offset);
}

vkShade::TimerUniform::TimerUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "timer"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::TimerUniform::update(VulkanBuffer& buffer, const ReshadeFrameState& frame)
{
    buffer.write(&frame.timer, m_size, m_offset);
}

vkShade::PingPongUniform::PingPongUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "pingpong"));

    auto to_float = [](const reshadefx::annotation& a, int idx = 0)
    {
        return a.type.is_floating_point() ? a.value.as_float[idx] : static_cast<float>(a.value.as_int[idx]);
    };

    for (const auto& a : uniform.annotations)
    {
        if      (a.name == "min")       m_state.min       = to_float(a);
        else if (a.name == "max")       m_state.max       = to_float(a);
        else if (a.name == "smoothing") m_state.smoothing = to_float(a);
        else if (a.name == "step")
        {
            m_state.stepMin = to_float(a, 0);
            m_state.stepMax = to_float(a, 1);
        }
    }

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::PingPongUniform::update(VulkanBuffer& buffer, const ReshadeFrameState& frame)
{
    m_state.advance(frame.frameTime * 0.001f, m_state.next_step(std::rand()));
    buffer.write(m_state.value.data(), m_size, m_offset);
}

vkShade::RandomUniform::RandomUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "random"));

    auto to_int = [](const reshadefx::annotation& a)
    {
        return a.type.is_integral() ? a.value.as_int[0] : static_cast<int>(a.value.as_float[0]);
    };

    for (const auto& a : uniform.annotations)
    {
        if      (a.name == "min") m_range.min = to_int(a);
        else if (a.name == "max") m_range.max = to_int(a);
    }

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::RandomUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    const int32_t value = m_range.value(static_cast<uint32_t>(std::rand()));
    buffer.write(&value, m_size, m_offset);
}

vkShade::KeyUniform::KeyUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "key"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::KeyUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle key press uniforms
    VkBool32 keyPressed = VK_FALSE;
    buffer.write(&keyPressed, m_size, m_offset);
}

vkShade::MouseButtonUniform::MouseButtonUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "mousebutton"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::MouseButtonUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle mouse button uniforms
    VkBool32 buttonPressed = VK_FALSE;
    buffer.write(&buttonPressed, m_size, m_offset);
}

vkShade::MousePointUniform::MousePointUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "mousepoint"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::MousePointUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle mouse point uniforms
    glm::vec2 mousePos {0.0f, 0.0f};
    buffer.write(&mousePos, m_size, m_offset);
}

vkShade::MouseDeltaUniform::MouseDeltaUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "mousedelta"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::MouseDeltaUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle mouse point uniforms
    glm::vec2 mouseDelta {0.0f, 0.0f};
    buffer.write(&mouseDelta, m_size, m_offset);
}

vkShade::MouseWheelUniform::MouseWheelUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "mousewheel"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::MouseWheelUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle mouse wheel uniforms
    glm::vec2 mouseWheel {0.0f, 0.0f};
    buffer.write(&mouseWheel, m_size, m_offset);
}

vkShade::DepthUniform::DepthUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "bufready_depth"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::DepthUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle depth ready uniforms
    VkBool32 hasDepth = VK_FALSE;
    buffer.write(&hasDepth, m_size, m_offset);
}

vkShade::OverlayOpenUniform::OverlayOpenUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "overlay_open"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::OverlayOpenUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle overlay open uniforms
    VkBool32 overlayOpen = VK_FALSE;
    buffer.write(&overlayOpen, m_size, m_offset);
}

vkShade::OverlayActiveUniform::OverlayActiveUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "overlay_active"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::OverlayActiveUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle overlay active uniforms
    int32_t overlayActive = 0;
    buffer.write(&overlayActive, m_size, m_offset);
}

vkShade::OverlayHoveredUniform::OverlayHoveredUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "overlay_hovered"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::OverlayHoveredUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle overlay hovered uniforms
    int32_t overlayHovered = 0;
    buffer.write(&overlayHovered, m_size, m_offset);
}

vkShade::ScreenshotUniform::ScreenshotUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "screenshot"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::ScreenshotUniform::update(VulkanBuffer& buffer, const ReshadeFrameState&)
{
    // TODO: Handle screenshot uniforms? (if in-scope)
    VkBool32 isScreenshot = VK_FALSE;
    buffer.write(&isScreenshot, m_size, m_offset);
}
