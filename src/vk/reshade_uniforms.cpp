#include "reshade_uniforms.hpp"

#include <chrono>
#include <effect_module.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

#include "vk/buffer.hpp"

vkShade::FrameTimeUniform::FrameTimeUniform(reshadefx::uniform uniform)
{
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "frametime")
        throw std::runtime_error("Tried to create a FrameTimeUniform from a non-frametime uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "framecount")
        throw std::runtime_error("Tried to create a FrameCountUniform from a non-framecount uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "date")
        throw std::runtime_error("Tried to create a DateUniform from a non-date uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "timer")
        throw std::runtime_error("Tried to create a TimerUniform from a non-timer uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "pingpong")
        throw std::runtime_error("Tried to create a PingPongUniform from a non-pingpong uniform");

    if (auto minAnnotation =
            std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a) { return a.name == "min"; });
        minAnnotation != uniform.annotations.end())
    {
        m_min = minAnnotation->type.is_floating_point() ? minAnnotation->value.as_float[0] : static_cast<float>(minAnnotation->value.as_int[0]);
    }
    if (auto maxAnnotation =
            std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a) { return a.name == "max"; });
        maxAnnotation != uniform.annotations.end())
    {
        m_max = maxAnnotation->type.is_floating_point() ? maxAnnotation->value.as_float[0] : static_cast<float>(maxAnnotation->value.as_int[0]);
    }
    if (auto smoothingAnnotation =
            std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a) { return a.name == "smoothing"; });
        smoothingAnnotation != uniform.annotations.end())
    {
        m_smoothing = smoothingAnnotation->type.is_floating_point() ? smoothingAnnotation->value.as_float[0]
                                                                  : static_cast<float>(smoothingAnnotation->value.as_int[0]);
    }
    if (auto stepAnnotation =
            std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a) { return a.name == "step"; });
        stepAnnotation != uniform.annotations.end())
    {
        m_stepMin =
            stepAnnotation->type.is_floating_point() ? stepAnnotation->value.as_float[0] : static_cast<float>(stepAnnotation->value.as_int[0]);
        m_stepMax =
            stepAnnotation->type.is_floating_point() ? stepAnnotation->value.as_float[1] : static_cast<float>(stepAnnotation->value.as_int[1]);
    }

    m_lastFrame = std::chrono::steady_clock::now();
    m_offset    = uniform.offset;
    m_size      = uniform.size;
}

void vkShade::PingPongUniform::update(VulkanBuffer& buffer)
{
    auto currentFrame = std::chrono::steady_clock::now();

    std::chrono::duration<float, std::ratio<1>> frameTime = currentFrame - m_lastFrame;

    float increment = m_stepMax == 0 ? m_stepMin : (m_stepMin + std::fmod(static_cast<float>(std::rand()), m_stepMax - m_stepMin + 1.0f));
    if (m_currentValue[1] >= 0)
    {
        increment = std::max(increment - std::max(0.0f, m_smoothing - (m_max - m_currentValue[0])), 0.05f);
        increment *= frameTime.count();

        if ((m_currentValue[0] += increment) >= m_max)
        {
            m_currentValue[0] = m_max, m_currentValue[1] = -1.0f;
        }
    }
    else
    {
        increment = std::max(increment - std::max(0.0f, m_smoothing - (m_currentValue[0] - m_min)), 0.05f);
        increment *= frameTime.count();

        if ((m_currentValue[0] -= increment) <= m_min)
        {
            m_currentValue[0] = m_min, m_currentValue[1] = 1.0f;
        }
    }

    buffer.write(m_currentValue, m_size, m_offset);
}

vkShade::RandomUniform::RandomUniform(reshadefx::uniform uniform)
{
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "random")
        throw std::runtime_error("Tried to create a RandomUniform from a non-random uniform");

    if (auto minAnnotation =
            std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a) { return a.name == "min"; });
        minAnnotation != uniform.annotations.end())
    {
        m_min = minAnnotation->type.is_integral() ? minAnnotation->value.as_int[0] : static_cast<int>(minAnnotation->value.as_float[0]);
    }
    if (auto maxAnnotation =
            std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a) { return a.name == "max"; });
        maxAnnotation != uniform.annotations.end())
    {
        m_max = maxAnnotation->type.is_integral() ? maxAnnotation->value.as_int[0] : static_cast<int>(maxAnnotation->value.as_float[0]);
    }

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::RandomUniform::update(VulkanBuffer& buffer)
{
    int32_t value = m_min + (std::rand() % (m_max - m_min + 1));
    buffer.write(&value, m_size, m_offset);
}

vkShade::KeyUniform::KeyUniform(reshadefx::uniform uniform)
{
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "key")
        throw std::runtime_error("Tried to create a KeyUniform from a non-key uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "mousebutton")
        throw std::runtime_error("Tried to create a MouseButtonUniform from a non-mousebutton uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "mousepoint")
        throw std::runtime_error("Tried to create a MousePointUniform from a non-mousepoint uniform");

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
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "mousedelta")
        throw std::runtime_error("Tried to create a MouseDeltaUniform from a non-mousedelta uniform");

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::MouseDeltaUniform::update(VulkanBuffer& buffer)
{
    // TODO: Handle mouse point uniforms
    glm::vec2 mouseDelta {0.0f, 0.0f};
    buffer.write(&mouseDelta, m_size, m_offset);
}

vkShade::DepthUniform::DepthUniform(reshadefx::uniform uniform)
{
    auto source = std::find_if(uniform.annotations.begin(), uniform.annotations.end(), [](const auto& a)
    {
        return a.name == "source";
    });

    if (source->value.string_data != "bufready_depth")
        throw std::runtime_error("Tried to create a MouseDeltaUniform from a non-bufready_depth uniform");

    m_offset = uniform.offset;
    m_size   = uniform.size;
}

void vkShade::DepthUniform::update(VulkanBuffer& buffer)
{
    // TODO: Handle depth ready uniforms
    VkBool32 hasDepth = VK_FALSE;
    buffer.write(&hasDepth, m_size, m_offset);
}
