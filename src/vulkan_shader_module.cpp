#include "vulkan_shader_module.hpp"

#include <fstream>

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "vulkan_hooks.hpp"

vkShade::VulkanShaderModule::VulkanShaderModule(VkDevice device, const std::string& filePath)
    : VulkanObject(device)
{
    // Store the device handle
    m_device = device;

    // Read file into temporary buffer
    std::ifstream file(filePath.c_str(), std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    // Create shader module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(m_device)];
    VkResult result = thisDevice.dispatch.CreateShaderModule(m_device, &createInfo, nullptr, &m_module);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module from: " + filePath);
    }

    spdlog::debug("Created shader module from: {}", filePath);
}

vkShade::VulkanShaderModule::~VulkanShaderModule()
{
    if (m_module != VK_NULL_HANDLE)
    {
        auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(m_device)];
        thisDevice.dispatch.DestroyShaderModule(m_device, m_module, nullptr);
        spdlog::debug("Destroyed shader module");
    }
}
