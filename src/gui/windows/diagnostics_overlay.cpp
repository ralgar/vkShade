#include "diagnostics_overlay.hpp"

#include <algorithm>

#include <imgui.h>

vkShade::DiagnosticsOverlay::DiagnosticsOverlay(
    std::shared_ptr<DiagnosticsState> diagnosticsState)
    : m_diagnosticsState(std::move(diagnosticsState))
{}

void vkShade::DiagnosticsOverlay::render()
{
    const bool showPerformance =
        m_diagnosticsState->showPerformanceOverlay.load(std::memory_order_relaxed);
    if (!showPerformance)
        return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    constexpr float margin = 12.0f;
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - margin,
               viewport->WorkPos.y + margin),
        ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.78f);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("vkShade HUD###diagnostics-overlay", nullptr, flags))
    {
        ImGui::TextUnformatted("vkShade");
        ImGui::Separator();

        render_performance();
    }
    ImGui::End();
}

void vkShade::DiagnosticsOverlay::render_performance()
{
    if (!m_diagnosticsState->gpuTimingSupported.load(std::memory_order_relaxed))
    {
        reset_performance_samples();
        ImGui::TextDisabled("GPU timing unavailable");
        return;
    }

    if (!m_diagnosticsState->gpuTimingValid.load(std::memory_order_acquire))
    {
        reset_performance_samples();
        ImGui::TextDisabled("GPU timing: waiting for sample");
        return;
    }

    const uint32_t sample =
        m_diagnosticsState->gpuTimingSampleSequence.load(std::memory_order_acquire);
    if (sample != m_lastGpuTimingSample)
    {
        const double total =
            m_diagnosticsState->totalGpuMilliseconds.load(std::memory_order_relaxed);
        const double effects =
            m_diagnosticsState->effectsGpuMilliseconds.load(std::memory_order_relaxed);
        m_totalGpuStatistics.add(total);
        m_effectsGpuStatistics.add(effects);
        m_otherGpuStatistics.add(std::max(0.0, total - effects));
        m_lastGpuTimingSample = sample;
    }

    const auto total = m_totalGpuStatistics.snapshot();
    const auto effects = m_effectsGpuStatistics.snapshot();
    const auto other = m_otherGpuStatistics.snapshot();
    if (!total || !effects || !other)
    {
        ImGui::TextDisabled("GPU timing: waiting for sample");
        return;
    }

    ImGui::TextDisabled("                 cur      avg      min      max");
    ImGui::Text("vkShade GPU  %7.3f  %7.3f  %7.3f  %7.3f ms",
                total->current, total->average, total->minimum, total->maximum);
    ImGui::TextDisabled("Effects      %7.3f  %7.3f  %7.3f  %7.3f ms",
                        effects->current, effects->average,
                        effects->minimum, effects->maximum);
    ImGui::TextDisabled("Other        %7.3f  %7.3f  %7.3f  %7.3f ms",
                        other->current, other->average, other->minimum, other->maximum);
}

void vkShade::DiagnosticsOverlay::reset_performance_samples()
{
    m_totalGpuStatistics.reset();
    m_effectsGpuStatistics.reset();
    m_otherGpuStatistics.reset();
    m_lastGpuTimingSample =
        m_diagnosticsState->gpuTimingSampleSequence.load(std::memory_order_relaxed);
}
