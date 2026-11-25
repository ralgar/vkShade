#pragma once

#include "platform/clipboard.hpp"
#include "wayland_client_context.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include <wayland-client.h>

namespace vkShade::Platform
{
    class WaylandClipboard final : public Clipboard
    {
    public:
        explicit WaylandClipboard(std::shared_ptr<WaylandClientState> state);
        ~WaylandClipboard() override;

        bool is_available() const;
        bool set_text(std::string_view text) override;
        void update() override;

        void on_registry_global(
            wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
        void on_data_offer(wl_data_offer* offer);
        void on_drag_enter(wl_data_offer* offer);
        void on_drag_leave();
        void on_selection(wl_data_offer* offer);
        void on_source_send(wl_data_source* source, const char* mimeType, int32_t fd);
        void on_source_cancelled(wl_data_source* source);

    private:
        void create_data_device();
        void destroy_offer(wl_data_offer* offer);
        void destroy_source();

        std::shared_ptr<WaylandClientState> m_state;
        WaylandClientContext m_context;
        wl_data_device_manager* m_manager = nullptr;
        wl_seat* m_seat = nullptr;
        wl_data_device* m_dataDevice = nullptr;
        wl_data_source* m_source = nullptr;
        wl_data_offer* m_dragOffer = nullptr;
        std::unordered_set<wl_data_offer*> m_offers;
        std::string m_text;
    };
} // namespace vkShade::Platform
