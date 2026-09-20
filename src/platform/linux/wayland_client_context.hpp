#pragma once

#include <atomic>
#include <memory>

#include <wayland-client.h>

namespace vkShade::Platform
{
    class WaylandClientState
    {
    public:
        explicit WaylandClientState(wl_display* display);

        [[nodiscard]] wl_display* get_display() const;
        [[nodiscard]] uint32_t get_input_serial() const;
        void set_input_serial(uint32_t serial);

    private:
        wl_display* m_display = nullptr;
        std::atomic<uint32_t> m_inputSerial {0};
    };

    class WaylandClientContext
    {
    public:
        explicit WaylandClientContext(wl_display* display);
        explicit WaylandClientContext(std::shared_ptr<WaylandClientState> state);
        ~WaylandClientContext();

        WaylandClientContext(const WaylandClientContext&) = delete;
        WaylandClientContext& operator=(const WaylandClientContext&) = delete;
        WaylandClientContext(WaylandClientContext&&) = delete;
        WaylandClientContext& operator=(WaylandClientContext&&) = delete;

        [[nodiscard]] bool is_available() const;
        [[nodiscard]] wl_registry* get_registry() const;
        [[nodiscard]] std::shared_ptr<WaylandClientState> get_state() const;

        void assign_queue(wl_proxy* proxy) const;
        bool synchronize() const;
        void dispatch_pending() const;
        void flush() const;

    private:
        std::shared_ptr<WaylandClientState> m_state;
        wl_display*     m_display = nullptr;
        wl_event_queue* m_queue = nullptr;
        wl_registry*    m_registry = nullptr;
    };
} // namespace vkShade::Platform
