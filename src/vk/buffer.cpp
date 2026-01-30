#include "buffer.hpp"

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

#include "macros.hpp"

vkShade::VulkanBuffer::VulkanBuffer(VulkanDevice& device, size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    : VulkanObject(device)
{
    if (size == 0)
        throw std::runtime_error("Cannot initialize buffer with size '0'!");

    spdlog::trace("Creating buffer ({} bytes, {})", size, magic_enum::enum_name(memoryUsage));

    // Allocate the buffer
	VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocationCreateInfo = {};
	allocationCreateInfo.usage = memoryUsage;

    // Set the mapped bit to enable writing directly to CPU or CPU-to-GPU buffers using pMappedData.
    if (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY || memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU)
	    allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // Create the buffer
	VK_CHECK(vmaCreateBuffer(m_device.allocator, &bufferInfo, &allocationCreateInfo, &m_buffer, &m_allocation, &m_allocationInfo));

    // Get the address of the buffer if flag is set
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo deviceAddressInfo {};
        deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAddressInfo.buffer = m_buffer;
        m_address = m_device.dispatch.GetBufferDeviceAddress(m_device.handle, &deviceAddressInfo);
    }
}

vkShade::VulkanBuffer::~VulkanBuffer()
{
    spdlog::trace("Destroying buffer");
    vmaDestroyBuffer(m_device.allocator, m_buffer, m_allocation);
}

bool vkShade::VulkanBuffer::copy(VkCommandBuffer cmd, VkBuffer source, size_t size, size_t srcOffset, size_t dstOffset)
{
    if (dstOffset + size > m_allocationInfo.size)
    {
        spdlog::error("Buffer copy would overflow destination");
        return false;
    }

    VkBufferCopy bufferCopy {0};
    bufferCopy.dstOffset = srcOffset;
    bufferCopy.srcOffset = dstOffset;
    bufferCopy.size = size;

    m_device.dispatch.CmdCopyBuffer(cmd, source, m_buffer, 1, &bufferCopy);
    return true;
}

bool vkShade::VulkanBuffer::write(const void* data, size_t size, size_t offset)
{
    // Make sure the buffer is mapped on the CPU
    if (!m_allocationInfo.pMappedData)
    {
        spdlog::error("Cannot write to unmapped buffer");
        return false;
    }

    if (offset + size > m_allocationInfo.size)
    {
        spdlog::error("Write would overflow buffer");
        return false;
    }

    // We don't need to map and unmap since we're using the mapped bit, we can just write directly.
	memcpy(static_cast<uint8_t*>(m_allocationInfo.pMappedData) + offset, data, size);
    return true;
}
