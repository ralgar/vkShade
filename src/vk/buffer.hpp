#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "hooks/hooks.hpp"
#include "object.hpp"

namespace vkShade
{
    class VulkanBuffer : public VulkanObject
    {
    public:
        VulkanBuffer(VulkanDevice& device, size_t size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage);
        ~VulkanBuffer() override;

        // Copy the source buffer into this one. Mostly used for uploading to GPU-only buffers.
        bool copy(VkCommandBuffer cmd, VkBuffer source, size_t size, size_t srcOffset = 0, size_t dstOffset = 0);

        // Write into a CPU-only or CPU-to-GPU buffer.
        bool write(const void* data, size_t size, size_t offset = 0);

        const VkDeviceAddress address() const { return m_address; }
        const VkBuffer& buffer() const { return m_buffer; }
        void* data() const { return m_allocationInfo.pMappedData; }
        size_t size() const { return m_allocationInfo.size; }

    private:
        VkBuffer          m_buffer;
        VkDeviceAddress   m_address {0};
        VmaAllocation     m_allocation;
        VmaAllocationInfo m_allocationInfo;
    };
} // namespace vkShade
