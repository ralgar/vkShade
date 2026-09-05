#include "application_mouse_inhibitors.hpp"

#include <chrono>
#include <cstdint>
#include <dlfcn.h>
#include <link.h>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/logger.hpp"
#include "mouse_input_inhibitor_group.hpp"
#include "sdl_module.hpp"

namespace
{
    struct SDLEventHeader
    {
        uint32_t type;
    };

    using SDLFlushEvents = void (*)(uint32_t, uint32_t);
    using SDLGetRelativeMouseMode = int (*)();
    using SDLSetRelativeMouseMode = int (*)(int);

    struct SDLWindow;
    using SDLWindowId = uint32_t;
    using SDLGetWindows = SDLWindow** (*)(int*);
    using SDLGetWindowRelativeMouseMode = bool (*)(SDLWindow*);
    using SDLSetWindowRelativeMouseMode = bool (*)(SDLWindow*, bool);
    using SDLGetWindowId = SDLWindowId (*)(SDLWindow*);
    using SDLGetWindowFromId = SDLWindow* (*)(SDLWindowId);
    using SDLFree = void (*)(void*);
    using SDLMainThreadCallback = void (*)(void*);
    using SDLRunOnMainThread = bool (*)(SDLMainThreadCallback, void*, bool);

    using NtUserGetRegisteredRawInputDevices = uint32_t (*)(void*, uint32_t*, uint32_t);
    using NtUserRegisterRawInputDevices = int (*)(const void*, uint32_t, uint32_t);
    using NtUserGetClipCursor = int (*)(void*);
    using NtUserClipCursor = int (*)(const void*);

    constexpr uint32_t SDL_MOUSEMOTION = 0x400;
    constexpr uint32_t SDL_MOUSEWHEEL = 0x403;
    constexpr uint32_t RAW_INPUT_ERROR = UINT32_MAX;
    constexpr uint32_t RIDEV_REMOVE = 0x00000001;

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

    template<typename FilterResult>
    class SDLEventFilter
    {
    public:
        using Callback = FilterResult (*)(void*, SDLEventHeader*);
        using Get = FilterResult (*)(Callback*, void**);
        using Set = void (*)(Callback, void*);

        explicit SDLEventFilter(vkShade::SdlModule& module)
            : m_module(module)
        {
        }

        bool install()
        {
            auto get = m_module.get_function<Get>("SDL_GetEventFilter");
            auto set = m_module.get_function<Set>("SDL_SetEventFilter");
            if (!get || !set)
                return false;

            Callback previous = nullptr;
            void* previousData = nullptr;
            get(&previous, &previousData);
            {
                const std::scoped_lock lock(m_mutex);
                m_previous = previous;
                m_previousData = previousData;
                m_installed = true;
            }
            set(filter_mouse_events, this);
            return true;
        }

        void reconcile()
        {
            auto get = m_module.get_function<Get>("SDL_GetEventFilter");
            auto set = m_module.get_function<Set>("SDL_SetEventFilter");
            if (!is_installed() || !get || !set)
                return;

            Callback current = nullptr;
            void* currentData = nullptr;
            get(&current, &currentData);
            if (current == filter_mouse_events && currentData == this)
                return;

            // SDL owns one process-global filter. Preserve a replacement as our
            // new predecessor so non-mouse events keep reaching application code.
            {
                const std::scoped_lock lock(m_mutex);
                m_previous = current;
                m_previousData = currentData;
            }
            set(filter_mouse_events, this);
            vkShade::Logger::debug("[InputManager] Reinstalled SDL mouse event filter");
        }

        void restore()
        {
            auto get = m_module.get_function<Get>("SDL_GetEventFilter");
            auto set = m_module.get_function<Set>("SDL_SetEventFilter");
            if (is_installed() && get && set)
            {
                Callback current = nullptr;
                void* currentData = nullptr;
                get(&current, &currentData);
                // Do not overwrite a filter installed after ours; ownership of
                // the process-global slot has already returned to the application.
                if (current == filter_mouse_events && currentData == this)
                {
                    Callback previous = nullptr;
                    void* previousData = nullptr;
                    {
                        const std::scoped_lock lock(m_mutex);
                        previous = m_previous;
                        previousData = m_previousData;
                    }
                    set(previous, previousData);
                    vkShade::Logger::debug("[InputManager] Restored SDL event filter");
                }
                else
                {
                    vkShade::Logger::debug(
                        "[InputManager] SDL event filter changed while overlay was active");
                }
            }

            const std::scoped_lock lock(m_mutex);
            m_installed = false;
            m_previous = nullptr;
            m_previousData = nullptr;
        }

        bool is_installed() const
        {
            const std::scoped_lock lock(m_mutex);
            return m_installed;
        }

    private:
        static FilterResult filter_mouse_events(void* opaque, SDLEventHeader* event)
        {
            if (event && event->type >= SDL_MOUSEMOTION && event->type <= SDL_MOUSEWHEEL)
                return {};

            Callback previous = nullptr;
            void* previousData = nullptr;
            {
                auto& filter = *static_cast<SDLEventFilter*>(opaque);
                const std::scoped_lock lock(filter.m_mutex);
                previous = filter.m_previous;
                previousData = filter.m_previousData;
            }
            return previous ? previous(previousData, event) : FilterResult {1};
        }

        vkShade::SdlModule& m_module;
        mutable std::mutex m_mutex;
        Callback m_previous {nullptr};
        void* m_previousData {nullptr};
        bool m_installed {false};
    };

    class SDL2MouseInputInhibitor final : public vkShade::MouseInputInhibitor
    {
    public:
        explicit SDL2MouseInputInhibitor(vkShade::SdlModule module)
            : m_module(std::move(module))
            , m_eventFilter(m_module)
        {
            vkShade::Logger::debug(
                "[InputManager] Found SDL2 input API in {}", m_module.get_path());
        }

        bool inhibit() override
        {
            auto getRelativeMouseMode =
                m_module.get_function<SDLGetRelativeMouseMode>("SDL_GetRelativeMouseMode");
            auto setRelativeMouseMode =
                m_module.get_function<SDLSetRelativeMouseMode>("SDL_SetRelativeMouseMode");
            auto flushEvents = m_module.get_function<SDLFlushEvents>("SDL_FlushEvents");

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

            if (m_eventFilter.install())
            {
                if (flushEvents)
                    flushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

                vkShade::Logger::debug("[InputManager] Installed SDL mouse event filter");
            }

            m_nextReconcile = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
            return m_restoreRelativeMode || m_eventFilter.is_installed();
        }

        void reconcile() override
        {
            const auto now = std::chrono::steady_clock::now();
            if (now < m_nextReconcile)
                return;
            m_nextReconcile = now + std::chrono::milliseconds(100);

            auto getRelativeMouseMode =
                m_module.get_function<SDLGetRelativeMouseMode>("SDL_GetRelativeMouseMode");
            auto setRelativeMouseMode =
                m_module.get_function<SDLSetRelativeMouseMode>("SDL_SetRelativeMouseMode");
            if (getRelativeMouseMode && setRelativeMouseMode && getRelativeMouseMode() != 0)
            {
                if (setRelativeMouseMode(0) == 0)
                    vkShade::Logger::debug(
                        "[InputManager] Re-disabled SDL relative mouse mode");
            }

            m_eventFilter.reconcile();
        }

        void restore() override
        {
            m_eventFilter.restore();

            auto setRelativeMouseMode =
                m_module.get_function<SDLSetRelativeMouseMode>("SDL_SetRelativeMouseMode");
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
        vkShade::SdlModule m_module;
        SDLEventFilter<int> m_eventFilter;
        bool m_restoreRelativeMode {false};
        std::chrono::steady_clock::time_point m_nextReconcile {};
    };

    struct SDL3Api
    {
        explicit SDL3Api(vkShade::SdlModule loadedModule)
            : module(std::move(loadedModule))
            , getWindows(module.get_function<SDLGetWindows>("SDL_GetWindows"))
            , getRelativeMouseMode(module.get_function<SDLGetWindowRelativeMouseMode>(
                  "SDL_GetWindowRelativeMouseMode"))
            , setRelativeMouseMode(module.get_function<SDLSetWindowRelativeMouseMode>(
                  "SDL_SetWindowRelativeMouseMode"))
            , getWindowId(module.get_function<SDLGetWindowId>("SDL_GetWindowID"))
            , getWindowFromId(module.get_function<SDLGetWindowFromId>("SDL_GetWindowFromID"))
            , free(module.get_function<SDLFree>("SDL_free"))
            , runOnMainThread(module.get_function<SDLRunOnMainThread>("SDL_RunOnMainThread"))
        {
        }

        bool is_available() const
        {
            return getWindows && getRelativeMouseMode && setRelativeMouseMode
                && getWindowId && getWindowFromId && free && runOnMainThread;
        }

        vkShade::SdlModule module;
        SDLGetWindows getWindows {nullptr};
        SDLGetWindowRelativeMouseMode getRelativeMouseMode {nullptr};
        SDLSetWindowRelativeMouseMode setRelativeMouseMode {nullptr};
        SDLGetWindowId getWindowId {nullptr};
        SDLGetWindowFromId getWindowFromId {nullptr};
        SDLFree free {nullptr};
        SDLRunOnMainThread runOnMainThread {nullptr};
    };

    class SDL3RelativeModeInhibitor
    {
    public:
        explicit SDL3RelativeModeInhibitor(std::shared_ptr<SDL3Api> api)
            : m_state(std::make_shared<State>(std::move(api)))
        {
        }

        bool inhibit()
        {
            uint64_t generation = 0;
            {
                const std::scoped_lock lock(m_state->mutex);
                m_state->requested = true;
                generation = ++m_state->generation;
                m_state->reconcilePending = true;
            }

            if (schedule(TaskAction::Inhibit, generation, {}))
                return true;

            const std::scoped_lock lock(m_state->mutex);
            if (m_state->generation == generation)
                m_state->reconcilePending = false;
            return false;
        }

        void reconcile()
        {
            uint64_t generation = 0;
            {
                const std::scoped_lock lock(m_state->mutex);
                if (!m_state->requested || m_state->reconcilePending)
                    return;

                generation = m_state->generation;
                m_state->reconcilePending = true;
            }

            if (schedule(TaskAction::Inhibit, generation, {}))
                return;

            const std::scoped_lock lock(m_state->mutex);
            if (m_state->generation == generation)
                m_state->reconcilePending = false;
        }

        void restore()
        {
            uint64_t generation = 0;
            std::vector<SDLWindowId> windowIds;
            {
                const std::scoped_lock lock(m_state->mutex);
                m_state->requested = false;
                generation = ++m_state->generation;
                m_state->reconcilePending = false;
                windowIds.assign(
                    m_state->restoreWindowIds.begin(), m_state->restoreWindowIds.end());
            }

            if (!windowIds.empty()
                && !schedule(TaskAction::Restore, generation, std::move(windowIds)))
            {
                vkShade::Logger::warn(
                    "[InputManager] Could not schedule SDL3 relative mouse mode restore");
            }
        }

    private:
        struct State
        {
            explicit State(std::shared_ptr<SDL3Api> sharedApi)
                : api(std::move(sharedApi))
            {
            }

            std::shared_ptr<SDL3Api> api;
            std::mutex mutex;
            std::unordered_set<SDLWindowId> restoreWindowIds;
            // Main-thread callbacks may outlive the request that scheduled
            // them; generations keep stale work from reversing newer state.
            uint64_t generation {0};
            bool requested {false};
            bool reconcilePending {false};
        };

        enum class TaskAction
        {
            Inhibit,
            Restore,
        };

        struct Task
        {
            std::shared_ptr<State> state;
            TaskAction action;
            uint64_t generation;
            std::vector<SDLWindowId> windowIds;
        };

        bool schedule(
            TaskAction action,
            uint64_t generation,
            std::vector<SDLWindowId> windowIds)
        {
            auto* task = new Task {
                .state = m_state,
                .action = action,
                .generation = generation,
                .windowIds = std::move(windowIds),
            };

            // SDL3 window relative-mode functions are main-thread-only. An
            // asynchronous handoff avoids deadlocking a present thread while
            // keeping the module and request state alive until SDL runs it.
            if (m_state->api->runOnMainThread(run_task, task, false))
                return true;

            delete task;
            return false;
        }

        static void run_task(void* opaque)
        {
            const std::unique_ptr<Task> task(static_cast<Task*>(opaque));
            if (task->action == TaskAction::Restore)
                restore_windows(*task);
            else
                inhibit_windows(*task);
        }

        static void inhibit_windows(const Task& task)
        {
            {
                const std::scoped_lock lock(task.state->mutex);
                if (!task.state->requested || task.state->generation != task.generation)
                    return;
            }

            std::vector<SDLWindowId> changedWindowIds;
            int windowCount = 0;
            SDLWindow** windows = task.state->api->getWindows(&windowCount);
            for (int index = 0; windows && index < windowCount; ++index)
            {
                SDLWindow* window = windows[index];
                if (!window || !task.state->api->getRelativeMouseMode(window))
                    continue;

                if (task.state->api->setRelativeMouseMode(window, false))
                    changedWindowIds.push_back(task.state->api->getWindowId(window));
                else
                    vkShade::Logger::warn(
                        "[InputManager] SDL_SetWindowRelativeMouseMode(false) failed");
            }
            task.state->api->free(windows);

            bool restoreImmediately = false;
            {
                const std::scoped_lock lock(task.state->mutex);
                if (task.state->requested)
                {
                    task.state->restoreWindowIds.insert(
                        changedWindowIds.begin(), changedWindowIds.end());
                }
                else
                {
                    restoreImmediately = true;
                }

                if (task.state->generation == task.generation)
                    task.state->reconcilePending = false;
            }

            if (restoreImmediately)
                restore_window_ids(*task.state->api, changedWindowIds);
        }

        static void restore_windows(const Task& task)
        {
            {
                const std::scoped_lock lock(task.state->mutex);
                if (task.state->requested)
                {
                    task.state->restoreWindowIds.insert(
                        task.windowIds.begin(), task.windowIds.end());
                    return;
                }
            }

            restore_window_ids(*task.state->api, task.windowIds);

            const std::scoped_lock lock(task.state->mutex);
            if (!task.state->requested)
            {
                for (SDLWindowId windowId : task.windowIds)
                    task.state->restoreWindowIds.erase(windowId);
            }
        }

        static void restore_window_ids(
            const SDL3Api& api,
            const std::vector<SDLWindowId>& windowIds)
        {
            for (SDLWindowId windowId : windowIds)
            {
                SDLWindow* window = api.getWindowFromId(windowId);
                if (window && !api.setRelativeMouseMode(window, true))
                {
                    vkShade::Logger::warn(
                        "[InputManager] SDL_SetWindowRelativeMouseMode(true) failed");
                }
            }
        }

        std::shared_ptr<State> m_state;
    };

    class SDL3MouseInputInhibitor final : public vkShade::MouseInputInhibitor
    {
    public:
        explicit SDL3MouseInputInhibitor(vkShade::SdlModule module)
            : m_api(std::make_shared<SDL3Api>(std::move(module)))
            , m_eventFilter(m_api->module)
            , m_relativeMode(m_api)
        {
            vkShade::Logger::debug(
                "[InputManager] Found SDL3 input API in {}", m_api->module.get_path());
        }

        bool inhibit() override
        {
            const bool relativeModeScheduled = m_api->is_available() && m_relativeMode.inhibit();
            const bool filterInstalled = m_eventFilter.install();
            auto flushEvents = m_api->module.get_function<SDLFlushEvents>("SDL_FlushEvents");
            if (filterInstalled && flushEvents)
                flushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

            m_nextReconcile = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
            return relativeModeScheduled || filterInstalled;
        }

        void reconcile() override
        {
            const auto now = std::chrono::steady_clock::now();
            if (now < m_nextReconcile)
                return;
            m_nextReconcile = now + std::chrono::milliseconds(100);

            if (m_api->is_available())
                m_relativeMode.reconcile();
            m_eventFilter.reconcile();
        }

        void restore() override
        {
            m_eventFilter.restore();
            if (m_api->is_available())
                m_relativeMode.restore();
        }

    private:
        std::shared_ptr<SDL3Api> m_api;
        SDLEventFilter<bool> m_eventFilter;
        SDL3RelativeModeInhibitor m_relativeMode;
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
    if (auto module = SdlModule::find(SdlAbi::Sdl2))
        group->add(std::make_unique<SDL2MouseInputInhibitor>(std::move(*module)));
    if (auto module = SdlModule::find(SdlAbi::Sdl3))
        group->add(std::make_unique<SDL3MouseInputInhibitor>(std::move(*module)));
    group->add(std::make_unique<WineMouseInputInhibitor>());
    return group;
}
