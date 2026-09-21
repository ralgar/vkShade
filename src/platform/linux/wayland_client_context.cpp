#include "wayland_client_context.hpp"

#include <utility>

#include "core/logger.hpp"

namespace vkShade::Platform
{
    WaylandClientState::WaylandClientState(wl_display* display)
        : m_display(display)
    {
    }

    wl_display* WaylandClientState::get_display() const
    {
        return m_display;
    }

    uint32_t WaylandClientState::get_input_serial() const
    {
        return m_inputSerial.load(std::memory_order_relaxed);
    }

    void WaylandClientState::set_input_serial(uint32_t serial)
    {
        m_inputSerial.store(serial, std::memory_order_relaxed);
    }

    WaylandClientContext::WaylandClientContext(wl_display* display)
        : WaylandClientContext(std::make_shared<WaylandClientState>(display))
    {
    }

    WaylandClientContext::WaylandClientContext(std::shared_ptr<WaylandClientState> state)
        : m_state(std::move(state))
        , m_display(m_state ? m_state->get_display() : nullptr)
    {
        if (!m_display)
            return;

        m_queue = wl_display_create_queue(m_display);
        if (!m_queue)
        {
            Logger::error("Failed to create Wayland event queue");
            return;
        }

        m_registry = wl_display_get_registry(m_display);
        if (!m_registry)
        {
            Logger::error("Failed to get Wayland registry");
            return;
        }

        assign_queue(reinterpret_cast<wl_proxy*>(m_registry));
    }

    WaylandClientContext::~WaylandClientContext()
    {
        if (m_registry)
            wl_registry_destroy(m_registry);
        if (m_queue)
            wl_event_queue_destroy(m_queue);
    }

    bool WaylandClientContext::is_available() const
    {
        return m_display && m_queue && m_registry;
    }

    wl_registry* WaylandClientContext::get_registry() const
    {
        return m_registry;
    }

    std::shared_ptr<WaylandClientState> WaylandClientContext::get_state() const
    {
        return m_state;
    }

    void WaylandClientContext::assign_queue(wl_proxy* proxy) const
    {
        if (proxy && m_queue)
            wl_proxy_set_queue(proxy, m_queue);
    }

    bool WaylandClientContext::synchronize() const
    {
        return is_available() && wl_display_roundtrip_queue(m_display, m_queue) >= 0;
    }

    void WaylandClientContext::dispatch_pending() const
    {
        if (is_available())
            wl_display_dispatch_queue_pending(m_display, m_queue);
    }

    void WaylandClientContext::flush() const
    {
        if (is_available())
            wl_display_flush(m_display);
    }
} // namespace vkShade::Platform
