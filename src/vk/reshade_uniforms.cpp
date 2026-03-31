#include "reshade_uniforms.hpp"

#include <chrono>
#include <effect_module.hpp>
#include <glm/vec2.hpp>
#include <random>
#include <spdlog/spdlog.h>

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

    m_lastFrame = std::chrono::steady_clock::now();
    m_offset    = uniform.offset;
    m_size      = uniform.size;
}

void vkShade::FrameTimeUniform::update(VulkanBuffer& buffer)
{
    auto currentFrame = std::chrono::steady_clock::now();
    std::chrono::duration<float> duration = currentFrame - m_lastFrame;
    m_lastFrame = currentFrame;
    float frametime = duration.count();

    buffer.write(&frametime, m_size, m_offset);
}

vkShade::FrameCountUniform::FrameCountUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "framecount"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::FrameCountUniform::update(VulkanBuffer& buffer)
{
    buffer.write(&m_count, m_size, m_offset);
    m_count++;
}

vkShade::DateUniform::DateUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "date"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::DateUniform::update(VulkanBuffer& buffer)
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

    m_startTime = std::chrono::steady_clock::now();
    m_offset    = uniform.offset;
    m_size      = uniform.size;
}

void vkShade::TimerUniform::update(VulkanBuffer& buffer)
{
    auto currentFrame = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentFrame - m_startTime);
    float timer = static_cast<float>(elapsed.count());

    buffer.write(&timer, m_size, m_offset);
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
        if      (a.name == "min")       m_min       = to_float(a);
        else if (a.name == "max")       m_max       = to_float(a);
        else if (a.name == "smoothing") m_smoothing = to_float(a);
        else if (a.name == "step")
        {
            m_stepMin = to_float(a, 0);
            m_stepMax = to_float(a, 1);
        }
    }

    m_lastFrame = std::chrono::steady_clock::now();
    m_offset    = uniform.offset;
    m_size      = uniform.size;
}

void vkShade::PingPongUniform::update(VulkanBuffer& buffer)
{
    // Compute delta time and update tracking
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - m_lastFrame).count();
    m_lastFrame = now;

    // Pick a step size
    float step = m_stepMin;
    if (m_stepMax != 0.0f)
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(m_stepMin, m_stepMax);
        step = dist(rng);
    }

    // If going forwards, calculate smoothing reduction based on how
    //  close we are to m_max, and apply it to step.
    if (m_currentValue[1] >= 0.0f)
    {
        float smooth = std::max(0.0f, m_smoothing - (m_max - m_currentValue[0]));
        m_currentValue[0] += std::max(step - smooth, 0.05f) * deltaTime;
        if (m_currentValue[0] >= m_max)
        {
            m_currentValue[0] = m_max;
            m_currentValue[1] = -1.0f;
        }
    }
    else  // If going backwards, same logic in reverse.
    {
        float smooth = std::max(0.0f, m_smoothing - (m_currentValue[0] - m_min));
        m_currentValue[0] -= std::max(step - smooth, 0.05f) * deltaTime;
        if (m_currentValue[0] <= m_min)
        {
            m_currentValue[0] = m_min;
            m_currentValue[1] = 1.0f;
        }
    }

    buffer.write(m_currentValue, m_size, m_offset);
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
        if      (a.name == "min") m_min = to_int(a);
        else if (a.name == "max") m_max = to_int(a);
    }

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::RandomUniform::update(VulkanBuffer& buffer)
{
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int32_t> dist(m_min, m_max);
    int32_t value = dist(rng);
    buffer.write(&value, m_size, m_offset);
}

vkShade::KeyUniform::KeyUniform(reshadefx::uniform uniform)
{
    assert(uniform_has_source(uniform, "key"));

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::KeyUniform::update(VulkanBuffer& buffer)
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

void vkShade::MouseButtonUniform::update(VulkanBuffer& buffer)
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

void vkShade::MousePointUniform::update(VulkanBuffer& buffer)
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

void vkShade::MouseDeltaUniform::update(VulkanBuffer& buffer)
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

void vkShade::MouseWheelUniform::update(VulkanBuffer& buffer)
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

void vkShade::DepthUniform::update(VulkanBuffer& buffer)
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

void vkShade::OverlayOpenUniform::update(VulkanBuffer& buffer)
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

void vkShade::OverlayActiveUniform::update(VulkanBuffer& buffer)
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

void vkShade::OverlayHoveredUniform::update(VulkanBuffer& buffer)
{
    // TODO: Handle overlay hovered uniforms
    int32_t overlayHovered = 0;
    buffer.write(&overlayHovered, m_size, m_offset);
}
