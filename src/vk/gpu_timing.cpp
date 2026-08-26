#include "gpu_timing.hpp"

#include <algorithm>
#include <array>

#include "core/logger.hpp"
#include "hooks/hooks.hpp"

vkShade::GpuTiming::GpuTiming(VulkanDevice& device,
                              std::shared_ptr<DiagnosticsState> state)
    : VulkanObject(device),
      m_state(std::move(state))
{}

vkShade::GpuTiming::~GpuTiming()
{
    if (m_queryPool != VK_NULL_HANDLE)
        m_device.dispatch.DestroyQueryPool(m_device.handle, m_queryPool, nullptr);
}

void vkShade::GpuTiming::collect_results()
{
    if (!m_pendingResults || m_queryPool == VK_NULL_HANDLE)
        return;

    struct QueryResult
    {
        uint64_t value;
        uint64_t available;
    };

    std::array<QueryResult, QueryCount> results {};
    const VkResult result = m_device.dispatch.GetQueryPoolResults(
        m_device.handle, m_queryPool, 0, QueryCount, sizeof(results), results.data(),
        sizeof(QueryResult), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

    m_pendingResults = false;
    if (result != VK_SUCCESS)
    {
        m_state->gpuTimingValid.store(false, std::memory_order_relaxed);
        if (result != VK_NOT_READY)
            Logger::warn("Unable to read GPU timing queries: {}", static_cast<int>(result));
        return;
    }

    const bool allQueriesAvailable = std::ranges::all_of(
        results, [](const QueryResult& query) { return query.available != 0; });
    if (!allQueriesAvailable)
    {
        m_state->gpuTimingValid.store(false, std::memory_order_relaxed);
        return;
    }

    m_state->totalGpuMilliseconds.store(
        milliseconds(results[TotalBegin].value, results[TotalEnd].value),
        std::memory_order_relaxed);
    m_state->effectsGpuMilliseconds.store(
        milliseconds(results[EffectsBegin].value, results[EffectsEnd].value),
        std::memory_order_relaxed);
    m_state->gpuTimingValid.store(true, std::memory_order_release);
}

bool vkShade::GpuTiming::begin_frame(VkCommandBuffer commandBuffer)
{
    m_recording = false;
    if (!m_state->showPerformanceOverlay.load(std::memory_order_relaxed))
    {
        m_state->gpuTimingValid.store(false, std::memory_order_relaxed);
        return false;
    }

    if (!ensure_query_pool())
        return false;

    m_device.dispatch.CmdResetQueryPool(commandBuffer, m_queryPool, 0, QueryCount);
    m_device.dispatch.CmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                        m_queryPool, TotalBegin);
    m_recording = true;
    return true;
}

void vkShade::GpuTiming::begin_effects(VkCommandBuffer commandBuffer)
{
    if (m_recording)
        m_device.dispatch.CmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                            m_queryPool, EffectsBegin);
}

void vkShade::GpuTiming::end_effects(VkCommandBuffer commandBuffer)
{
    if (m_recording)
        m_device.dispatch.CmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                            m_queryPool, EffectsEnd);
}

void vkShade::GpuTiming::end_frame(VkCommandBuffer commandBuffer)
{
    if (!m_recording)
        return;

    m_device.dispatch.CmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                        m_queryPool, TotalEnd);
    m_pendingResults = true;
    m_recording = false;
}

bool vkShade::GpuTiming::ensure_query_pool()
{
    if (m_queryPool != VK_NULL_HANDLE)
        return true;

    if (m_creationAttempted || !m_state->gpuTimingSupported.load(std::memory_order_relaxed))
        return false;

    m_creationAttempted = true;
    const VkQueryPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = QueryCount,
    };

    const VkResult result = m_device.dispatch.CreateQueryPool(
        m_device.handle, &createInfo, nullptr, &m_queryPool);
    if (result != VK_SUCCESS)
    {
        m_state->gpuTimingSupported.store(false, std::memory_order_relaxed);
        Logger::warn("Unable to create GPU timing query pool: {}", static_cast<int>(result));
        return false;
    }

    return true;
}

double vkShade::GpuTiming::milliseconds(uint64_t begin, uint64_t end) const
{
    const uint64_t ticks = timestamp_tick_delta(begin, end, m_device.timestampValidBits);
    return static_cast<double>(ticks) * static_cast<double>(m_device.timestampPeriod) / 1'000'000.0;
}
