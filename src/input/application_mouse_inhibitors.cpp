#include "application_mouse_inhibitors.hpp"

#include <chrono>
#include <cstdint>
#include <dlfcn.h>
#include <link.h>
#include <string_view>
#include <vector>

#include "core/logger.hpp"
#include "mouse_input_inhibitor_group.hpp"

namespace
{
    struct SDLEventHeader
    {
        uint32_t type;
    };

    using SDLEventFilter = int (*)(void*, SDLEventHeader*);
    using SDLGetEventFilter = int (*)(SDLEventFilter*, void**);
    using SDLSetEventFilter = void (*)(SDLEventFilter, void*);
    using SDLFlushEvents = void (*)(uint32_t, uint32_t);
    using SDLGetRelativeMouseMode = int (*)();
    using SDLSetRelativeMouseMode = int (*)(int);

    using NtUserGetRegisteredRawInputDevices = uint32_t (*)(void*, uint32_t*, uint32_t);
    using NtUserRegisterRawInputDevices = int (*)(const void*, uint32_t, uint32_t);
    using NtUserGetClipCursor = int (*)(void*);
    using NtUserClipCursor = int (*)(const void*);

    constexpr uint32_t SDL_MOUSEMOTION = 0x400;
    constexpr uint32_t SDL_MOUSEWHEEL = 0x403;
    constexpr uint32_t RAW_INPUT_ERROR = UINT32_MAX;
    constexpr uint32_t RIDEV_REMOVE = 0x00000001;

    template<typename Function>
    Function find_default_function(const char* name)
    {
        return reinterpret_cast<Function>(dlsym(RTLD_DEFAULT, name));
    }

    struct WineModuleSearch
    {
        std::string_view suffix;
        const char* path {nullptr};
    };

    int find_wine_module(dl_phdr_info* info, size_t, void* opaque)
    {
        auto& search = *static_cast<WineModuleSearch*>(opaque);
        const std::string_view path = info->dlpi_name ? info->dlpi_name : "";
        if (!path.ends_with(search.suffix))
            return 0;

        search.path = info->dlpi_name;
        return 1;
    }

    template<typename Function>
    Function find_wine_function(const char* module, const char* name)
    {
        // Native processes must not gain Wine dependencies merely because the
        // inhibitor probes for an API that is meaningful only under Wine.
        WineModuleSearch search {module};
        dl_iterate_phdr(find_wine_module, &search);
        if (!search.path)
            return nullptr;

        void* handle = dlopen(search.path, RTLD_NOW | RTLD_NOLOAD);
        if (!handle)
            return nullptr;

        Function function = reinterpret_cast<Function>(dlsym(handle, name));
        dlclose(handle);
        return function;
    }

    class SDLMouseInputInhibitor final : public vkShade::MouseInputInhibitor
    {
    public:
        bool inhibit() override
        {
            auto getRelativeMouseMode =
                find_default_function<SDLGetRelativeMouseMode>("SDL_GetRelativeMouseMode");
            auto setRelativeMouseMode =
                find_default_function<SDLSetRelativeMouseMode>("SDL_SetRelativeMouseMode");
            auto getEventFilter =
                find_default_function<SDLGetEventFilter>("SDL_GetEventFilter");
            auto setEventFilter =
                find_default_function<SDLSetEventFilter>("SDL_SetEventFilter");
            auto flushEvents = find_default_function<SDLFlushEvents>("SDL_FlushEvents");

            if (getRelativeMouseMode && setRelativeMouseMode && getRelativeMouseMode() != 0)
            {
                if (setRelativeMouseMode(0) == 0)
                {
                    m_restoreRelativeMode = true;
                    vkShade::Logger::debug("[InputManager] Disabled SDL relative mouse mode");
                }
                else
                {
                    vkShade::Logger::warn("[InputManager] SDL_SetRelativeMouseMode(false) failed");
                }
            }

            if (getEventFilter && setEventFilter)
            {
                m_previousFilter = nullptr;
                m_previousFilterData = nullptr;
                getEventFilter(&m_previousFilter, &m_previousFilterData);
                setEventFilter(filter_mouse_events, this);
                m_filterInstalled = true;

                if (flushEvents)
                    flushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

                vkShade::Logger::debug("[InputManager] Installed SDL mouse event filter");
            }

            m_nextReconcile = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
            return m_restoreRelativeMode || m_filterInstalled;
        }

        void reconcile() override
        {
            const auto now = std::chrono::steady_clock::now();
            if (now < m_nextReconcile)
                return;
            m_nextReconcile = now + std::chrono::milliseconds(100);

            auto getRelativeMouseMode =
                find_default_function<SDLGetRelativeMouseMode>("SDL_GetRelativeMouseMode");
            auto setRelativeMouseMode =
                find_default_function<SDLSetRelativeMouseMode>("SDL_SetRelativeMouseMode");
            if (getRelativeMouseMode && setRelativeMouseMode && getRelativeMouseMode() != 0)
            {
                if (setRelativeMouseMode(0) == 0)
                    vkShade::Logger::debug(
                        "[InputManager] Re-disabled SDL relative mouse mode");
            }

            auto getEventFilter =
                find_default_function<SDLGetEventFilter>("SDL_GetEventFilter");
            auto setEventFilter =
                find_default_function<SDLSetEventFilter>("SDL_SetEventFilter");
            if (!m_filterInstalled || !getEventFilter || !setEventFilter)
                return;

            SDLEventFilter currentFilter = nullptr;
            void* currentFilterData = nullptr;
            getEventFilter(&currentFilter, &currentFilterData);
            if (currentFilter == filter_mouse_events && currentFilterData == this)
                return;

            // SDL owns one process-global filter. Preserve a replacement as our
            // new predecessor so non-mouse events keep reaching application code.
            m_previousFilter = currentFilter;
            m_previousFilterData = currentFilterData;
            setEventFilter(filter_mouse_events, this);
            vkShade::Logger::debug("[InputManager] Reinstalled SDL mouse event filter");
        }

        void restore() override
        {
            auto getEventFilter =
                find_default_function<SDLGetEventFilter>("SDL_GetEventFilter");
            auto setEventFilter =
                find_default_function<SDLSetEventFilter>("SDL_SetEventFilter");
            if (m_filterInstalled && getEventFilter && setEventFilter)
            {
                SDLEventFilter currentFilter = nullptr;
                void* currentFilterData = nullptr;
                getEventFilter(&currentFilter, &currentFilterData);
                // Do not overwrite a filter installed after ours; ownership of
                // the process-global slot has already returned to the application.
                if (currentFilter == filter_mouse_events && currentFilterData == this)
                {
                    setEventFilter(m_previousFilter, m_previousFilterData);
                    vkShade::Logger::debug("[InputManager] Restored SDL event filter");
                }
                else
                {
                    vkShade::Logger::debug(
                        "[InputManager] SDL event filter changed while overlay was active");
                }
            }

            m_filterInstalled = false;
            m_previousFilter = nullptr;
            m_previousFilterData = nullptr;

            auto setRelativeMouseMode =
                find_default_function<SDLSetRelativeMouseMode>("SDL_SetRelativeMouseMode");
            if (m_restoreRelativeMode && setRelativeMouseMode)
            {
                if (setRelativeMouseMode(1) == 0)
                    vkShade::Logger::debug("[InputManager] Restored SDL relative mouse mode");
                else
                    vkShade::Logger::warn("[InputManager] SDL_SetRelativeMouseMode(true) failed");
            }
            m_restoreRelativeMode = false;
        }

    private:
        static int filter_mouse_events(void* opaque, SDLEventHeader* event)
        {
            auto& inhibitor = *static_cast<SDLMouseInputInhibitor*>(opaque);
            if (event && event->type >= SDL_MOUSEMOTION && event->type <= SDL_MOUSEWHEEL)
                return 0;

            return inhibitor.m_previousFilter
                ? inhibitor.m_previousFilter(inhibitor.m_previousFilterData, event)
                : 1;
        }

        SDLEventFilter m_previousFilter {nullptr};
        void* m_previousFilterData {nullptr};
        bool m_restoreRelativeMode {false};
        bool m_filterInstalled {false};
        std::chrono::steady_clock::time_point m_nextReconcile {};
    };

    class WineMouseInputInhibitor final : public vkShade::MouseInputInhibitor
    {
    public:
        bool inhibit() override
        {
            release_cursor_clip();
            suppress_raw_input();
            return m_cursorClipReleased || m_rawInputRemoved;
        }

        void reconcile() override
        {
            const auto now = std::chrono::steady_clock::now();
            if (now < m_nextReconcile)
                return;
            m_nextReconcile = now + std::chrono::milliseconds(100);

            release_cursor_clip();
            suppress_raw_input();
        }

        void restore() override
        {
            restore_raw_input();
            restore_cursor_clip();
        }

    private:
        struct RawInputRegistration
        {
            uint16_t usagePage;
            uint16_t usage;
            uint32_t flags;
            void* targetWindow;
        };

        // Wine fills this ABI structure through dynamically resolved functions.
        // cppcheck-suppress-begin unusedStructMember
        struct WineRect
        {
            int32_t left;
            int32_t top;
            int32_t right;
            int32_t bottom;
        };
        // cppcheck-suppress-end unusedStructMember

        void release_cursor_clip()
        {
            auto getClipCursor = find_wine_function<NtUserGetClipCursor>(
                "/win32u.so", "NtUserGetClipCursor");
            auto clipCursor = find_wine_function<NtUserClipCursor>(
                "/win32u.so", "NtUserClipCursor");
            if (!getClipCursor || !clipCursor)
                return;

            if (!m_cursorClipReleased && !getClipCursor(&m_savedClip))
                return;

            if (clipCursor(nullptr))
            {
                if (!m_cursorClipReleased)
                    vkShade::Logger::debug("[InputManager] Released Wine cursor clipping");
                m_cursorClipReleased = true;
            }
        }

        void restore_cursor_clip()
        {
            if (!m_cursorClipReleased)
                return;

            auto clipCursor = find_wine_function<NtUserClipCursor>(
                "/win32u.so", "NtUserClipCursor");
            if (clipCursor && clipCursor(&m_savedClip))
                vkShade::Logger::debug("[InputManager] Restored Wine cursor clipping");
            else
                vkShade::Logger::warn("[InputManager] Failed to restore Wine cursor clipping");

            m_cursorClipReleased = false;
        }

        void suppress_raw_input()
        {
            auto getRegistrations = find_wine_function<NtUserGetRegisteredRawInputDevices>(
                "/win32u.so", "NtUserGetRegisteredRawInputDevices");
            auto setRegistrations = find_wine_function<NtUserRegisterRawInputDevices>(
                "/win32u.so", "NtUserRegisterRawInputDevices");
            if (!getRegistrations || !setRegistrations)
                return;

            uint32_t count = 0;
            if (getRegistrations(nullptr, &count, sizeof(RawInputRegistration)) == RAW_INPUT_ERROR
                || count == 0)
                return;

            std::vector<RawInputRegistration> registrations(count);
            uint32_t capacity = count;
            if (getRegistrations(registrations.data(), &capacity,
                                 sizeof(RawInputRegistration)) == RAW_INPUT_ERROR)
                return;

            for (uint32_t index = 0; index < capacity; ++index)
            {
                const auto& registration = registrations[index];
                if (registration.usagePage != 0x01 || registration.usage != 0x02)
                    continue;

                if (m_savedRawInput.empty())
                    m_savedRawInput.push_back(registration);
                RawInputRegistration removal {
                    .usagePage = 0x01,
                    .usage = 0x02,
                    .flags = RIDEV_REMOVE,
                    .targetWindow = nullptr,
                };
                if (setRegistrations(&removal, 1, sizeof(removal)))
                {
                    if (!m_rawInputRemoved)
                    {
                        vkShade::Logger::debug(
                            "[InputManager] Removed Wine mouse Raw Input registration");
                    }
                    m_rawInputRemoved = true;
                }
                else
                {
                    if (!m_rawInputRemoved)
                        m_savedRawInput.clear();
                    vkShade::Logger::warn(
                        "[InputManager] Failed to remove Wine mouse Raw Input registration");
                }
                break;
            }
        }

        void restore_raw_input()
        {
            if (!m_rawInputRemoved)
                return;

            auto setRegistrations = find_wine_function<NtUserRegisterRawInputDevices>(
                "/win32u.so", "NtUserRegisterRawInputDevices");
            if (setRegistrations && setRegistrations(
                    m_savedRawInput.data(),
                    static_cast<uint32_t>(m_savedRawInput.size()),
                    sizeof(RawInputRegistration)))
            {
                vkShade::Logger::debug(
                    "[InputManager] Restored Wine mouse Raw Input registration");
            }
            else
            {
                vkShade::Logger::warn(
                    "[InputManager] Failed to restore Wine mouse Raw Input registration");
            }

            m_savedRawInput.clear();
            m_rawInputRemoved = false;
        }

        WineRect m_savedClip {};
        std::vector<RawInputRegistration> m_savedRawInput;
        bool m_cursorClipReleased {false};
        bool m_rawInputRemoved {false};
        std::chrono::steady_clock::time_point m_nextReconcile {};
    };
}

std::unique_ptr<vkShade::MouseInputInhibitor> vkShade::create_application_mouse_inhibitor()
{
    auto group = std::make_unique<MouseInputInhibitorGroup>();
    group->add(std::make_unique<SDLMouseInputInhibitor>());
    group->add(std::make_unique<WineMouseInputInhibitor>());
    return group;
}
