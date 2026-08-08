#include "shader_module.hpp"

#include <filesystem>
#include <fstream>

#include <magic_enum/magic_enum.hpp>
#include "core/logger.hpp"

#include "hooks/hooks.hpp"

vkShade::ShaderModule::ShaderModule(VulkanDevice& device, const std::string& filePath)
    : VulkanObject(device),
      m_filePath(filePath)
{
    if (!std::filesystem::exists(m_filePath) || !std::filesystem::is_regular_file(m_filePath))
    {
        Logger::error("File does not exist: {}", m_filePath);
        m_valid = false;
    }
}

vkShade::ShaderModule::ShaderModule(VulkanDevice& device, std::span<const uint32_t> bytecode)
    : VulkanObject(device)
{
    // Create shader module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytecode.size_bytes();
    createInfo.pCode = bytecode.data();

    VkResult result = m_device.dispatch.CreateShaderModule(m_device.handle, &createInfo, nullptr, &m_module);

    if (result != VK_SUCCESS)
    {
        Logger::error("Failed to create shader module: {}", m_filePath);
    }

    this->set_ready(true);
    Logger::trace("Created VkShaderModule from bytecode");
}

bool vkShade::ShaderModule::load()
{
    // Read file into temporary buffer
    std::ifstream file(m_filePath.c_str(), std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        Logger::error("Failed to open file: {}", m_filePath);
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

    VkResult result = m_device.dispatch.CreateShaderModule(m_device.handle, &createInfo, nullptr, &m_module);

    if (result != VK_SUCCESS)
    {
        Logger::error("Failed to create shader module: {}", m_filePath);
        return false;
    }

    this->set_ready(true);
    Logger::trace("Created VkShaderModule from: {}", m_filePath);
    return true;
}

vkShade::ShaderModule::~ShaderModule()
{
    if (m_module != VK_NULL_HANDLE)
    {
        m_device.dispatch.DestroyShaderModule(m_device.handle, m_module, nullptr);
        Logger::trace("Destroyed VkShaderModule");
    }
}
