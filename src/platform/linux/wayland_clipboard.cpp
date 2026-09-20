#include "wayland_clipboard.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "core/logger.hpp"

namespace
{
    constexpr const char* utf8MimeType = "text/plain;charset=utf-8";
    constexpr const char* textMimeType = "text/plain";

    void write_clipboard_pipe(int32_t fd, std::string_view text)
    {
        sigset_t blockedSignals;
        sigset_t previousSignals;
        sigemptyset(&blockedSignals);
        sigaddset(&blockedSignals, SIGPIPE);
        const bool signalBlocked = pthread_sigmask(
            SIG_BLOCK, &blockedSignals, &previousSignals) == 0;

        bool brokenPipe = false;
        while (!text.empty())
        {
            const ssize_t written = write(fd, text.data(), text.size());
            if (written > 0)
            {
                text.remove_prefix(static_cast<size_t>(written));
                continue;
            }
            if (written < 0 && errno == EINTR)
                continue;
            brokenPipe = written < 0 && errno == EPIPE;
            break;
        }
        close(fd);

        if (signalBlocked)
        {
            if (brokenPipe && sigismember(&previousSignals, SIGPIPE) == 0)
            {
                timespec timeout {};
                sigtimedwait(&blockedSignals, nullptr, &timeout);
            }
            pthread_sigmask(SIG_SETMASK, &previousSignals, nullptr);
        }
    }

    const wl_data_offer_listener dataOfferListener = {
        .offer = [](void*, wl_data_offer*, const char*) {},
        .source_actions = [](void*, wl_data_offer*, uint32_t) {},
        .action = [](void*, wl_data_offer*, uint32_t) {},
    };

    const wl_data_source_listener dataSourceListener = {
        .target = [](void*, wl_data_source*, const char*) {},
        .send = [](void* data, wl_data_source* source, const char* mimeType, int32_t fd)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_source_send(
                source, mimeType, fd);
        },
        .cancelled = [](void* data, wl_data_source* source)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_source_cancelled(source);
        },
        .dnd_drop_performed = [](void*, wl_data_source*) {},
        .dnd_finished = [](void*, wl_data_source*) {},
        .action = [](void*, wl_data_source*, uint32_t) {},
    };

    const wl_data_device_listener dataDeviceListener = {
        .data_offer = [](void* data, wl_data_device*, wl_data_offer* offer)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_data_offer(offer);
        },
        .enter = [](void* data, wl_data_device*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t,
                    wl_data_offer* offer)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_drag_enter(offer);
        },
        .leave = [](void* data, wl_data_device*)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_drag_leave();
        },
        .motion = [](void*, wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t) {},
        .drop = [](void* data, wl_data_device*)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_drag_leave();
        },
        .selection = [](void* data, wl_data_device*, wl_data_offer* offer)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_selection(offer);
        },
    };

    const wl_registry_listener registryListener = {
        .global = [](void* data, wl_registry* registry, uint32_t name,
                     const char* interface, uint32_t version)
        {
            static_cast<vkShade::Platform::WaylandClipboard*>(data)->on_registry_global(
                registry, name, interface, version);
        },
        .global_remove = [](void*, wl_registry*, uint32_t) {},
    };
} // namespace

namespace vkShade::Platform
{
    WaylandClipboard::WaylandClipboard(std::shared_ptr<WaylandClientState> state)
        : m_state(std::move(state))
        , m_context(m_state)
    {
        if (!m_context.is_available())
            return;

        wl_registry_add_listener(m_context.get_registry(), &registryListener, this);
        if (!m_context.synchronize())
            Logger::error("Failed to initialize Wayland clipboard globals");
    }

    WaylandClipboard::~WaylandClipboard()
    {
        destroy_source();
        for (wl_data_offer* offer : m_offers)
            wl_data_offer_destroy(offer);
        m_offers.clear();

        if (m_dataDevice)
        {
            if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_dataDevice))
                >= WL_DATA_DEVICE_RELEASE_SINCE_VERSION)
                wl_data_device_release(m_dataDevice);
            else
                wl_data_device_destroy(m_dataDevice);
        }
        if (m_seat)
        {
            if (wl_proxy_get_version(reinterpret_cast<wl_proxy*>(m_seat))
                >= WL_SEAT_RELEASE_SINCE_VERSION)
                wl_seat_release(m_seat);
            else
                wl_seat_destroy(m_seat);
        }
        if (m_manager)
            wl_data_device_manager_destroy(m_manager);
    }

    bool WaylandClipboard::is_available() const
    {
        return m_dataDevice != nullptr;
    }

    bool WaylandClipboard::set_text(std::string_view text)
    {
        if (!is_available())
            return false;

        const uint32_t serial = m_state->get_input_serial();
        if (serial == 0)
        {
            Logger::warn("Cannot set Wayland clipboard before an input event provides a serial");
            return false;
        }

        wl_data_source* source = wl_data_device_manager_create_data_source(m_manager);
        if (!source)
            return false;

        m_context.assign_queue(reinterpret_cast<wl_proxy*>(source));
        wl_data_source_add_listener(source, &dataSourceListener, this);
        wl_data_source_offer(source, utf8MimeType);
        wl_data_source_offer(source, textMimeType);

        destroy_source();
        m_text.assign(text);
        m_source = source;
        wl_data_device_set_selection(m_dataDevice, m_source, serial);
        m_context.flush();
        return true;
    }

    void WaylandClipboard::update()
    {
        m_context.dispatch_pending();
    }

    void WaylandClipboard::on_registry_global(
        wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
    {
        if (!m_manager && std::strcmp(interface, wl_data_device_manager_interface.name) == 0)
        {
            m_manager = static_cast<wl_data_device_manager*>(wl_registry_bind(
                registry, name, &wl_data_device_manager_interface, std::min(version, 3u)));
            m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_manager));
            create_data_device();
        }
        else if (!m_seat && std::strcmp(interface, wl_seat_interface.name) == 0)
        {
            m_seat = static_cast<wl_seat*>(wl_registry_bind(
                registry, name, &wl_seat_interface, std::min(version, 5u)));
            m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_seat));
            create_data_device();
        }
    }

    void WaylandClipboard::on_data_offer(wl_data_offer* offer)
    {
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(offer));
        wl_data_offer_add_listener(offer, &dataOfferListener, this);
        m_offers.insert(offer);
    }

    void WaylandClipboard::on_drag_enter(wl_data_offer* offer)
    {
        m_dragOffer = offer;
    }

    void WaylandClipboard::on_drag_leave()
    {
        destroy_offer(m_dragOffer);
        m_dragOffer = nullptr;
    }

    void WaylandClipboard::on_selection(wl_data_offer* offer)
    {
        destroy_offer(offer);
    }

    void WaylandClipboard::on_source_send(
        wl_data_source* source, const char* mimeType, int32_t fd)
    {
        if (source != m_source
            || (std::strcmp(mimeType, utf8MimeType) != 0
                && std::strcmp(mimeType, textMimeType) != 0))
        {
            close(fd);
            return;
        }

        write_clipboard_pipe(fd, m_text);
    }

    void WaylandClipboard::on_source_cancelled(wl_data_source* source)
    {
        if (source != m_source)
            return;
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }

    void WaylandClipboard::create_data_device()
    {
        if (m_dataDevice || !m_manager || !m_seat)
            return;

        m_dataDevice = wl_data_device_manager_get_data_device(m_manager, m_seat);
        m_context.assign_queue(reinterpret_cast<wl_proxy*>(m_dataDevice));
        wl_data_device_add_listener(m_dataDevice, &dataDeviceListener, this);
    }

    void WaylandClipboard::destroy_offer(wl_data_offer* offer)
    {
        if (!offer || !m_offers.erase(offer))
            return;
        wl_data_offer_destroy(offer);
    }

    void WaylandClipboard::destroy_source()
    {
        if (!m_source)
            return;
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }
} // namespace vkShade::Platform
