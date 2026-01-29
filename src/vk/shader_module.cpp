#include "shader_module.hpp"

#include <filesystem>
#include <fstream>

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "hooks/hooks.hpp"

vkShade::ShaderModule::ShaderModule(VkDevice device, const std::string& filePath)
    : VulkanObject(device)
{
    m_device = device;
    m_filePath = filePath;

    if (!std::filesystem::exists(m_filePath) || !std::filesystem::is_regular_file(m_filePath))
    {
        spdlog::error("File does not exist: {}", m_filePath);
        m_valid = false;
    }
}

bool vkShade::ShaderModule::load()
{
    // Read file into temporary buffer
    std::ifstream file(m_filePath.c_str(), std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        spdlog::error("Failed to open file: {}", m_filePath);
        return false;
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
        spdlog::error("Failed to create shader module: {}", m_filePath);
        return false;
    }

    this->set_ready(true);
    spdlog::trace("Created VkShaderModule from: {}", m_filePath);
    return true;
}

vkShade::ShaderModule::~ShaderModule()
{
    if (m_module != VK_NULL_HANDLE)
    {
        auto& thisDevice = g_vulkanDevices[dispatch_key_from_handle(m_device)];
        thisDevice.dispatch.DestroyShaderModule(m_device, m_module, nullptr);
        spdlog::trace("Destroyed VkShaderModule");
    }
}
