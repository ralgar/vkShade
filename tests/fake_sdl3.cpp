#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

namespace
{
    struct EventHeader
    {
        // cppcheck-suppress unusedStructMember
        uint32_t type;
    };

    using EventFilter = bool (*)(void*, EventHeader*);

    int windowStorage;
    bool relativeMouseMode {true};
    EventFilter eventFilter {nullptr};
    void* eventFilterData {nullptr};
    bool runCallbacksImmediately {true};
    std::vector<std::pair<void (*)(void*), void*>> mainThreadCallbacks;
}

extern "C"
{
    void** SDL_GetWindows(int* count)
    {
        *count = 1;
        auto** windows = static_cast<void**>(std::malloc(sizeof(void*)));
        if (!windows)
            return nullptr;
        windows[0] = &windowStorage;
        return windows;
    }

    bool SDL_GetWindowRelativeMouseMode(void*)
    {
        return relativeMouseMode;
    }

    bool SDL_SetWindowRelativeMouseMode(void*, bool enabled)
    {
        relativeMouseMode = enabled;
        return true;
    }

    uint32_t SDL_GetWindowID(void*)
    {
        return 42;
    }

    void* SDL_GetWindowFromID(uint32_t id)
    {
        return id == 42 ? &windowStorage : nullptr;
    }

    void SDL_free(void* memory)
    {
        std::free(memory);
    }

    bool SDL_RunOnMainThread(void (*callback)(void*), void* data, bool)
    {
        if (runCallbacksImmediately)
            callback(data);
        else
            mainThreadCallbacks.emplace_back(callback, data);
        return true;
    }

    bool SDL_GetEventFilter(EventFilter* filter, void** data)
    {
        *filter = eventFilter;
        *data = eventFilterData;
        return eventFilter != nullptr;
    }

    void SDL_SetEventFilter(EventFilter filter, void* data)
    {
        eventFilter = filter;
        eventFilterData = data;
    }

    void SDL_FlushEvents(uint32_t, uint32_t)
    {
    }

    void FakeSDL3Reset()
    {
        relativeMouseMode = true;
        eventFilter = nullptr;
        eventFilterData = nullptr;
        runCallbacksImmediately = true;
        mainThreadCallbacks.clear();
    }

    bool FakeSDL3RelativeMouseMode()
    {
        return relativeMouseMode;
    }

    bool FakeSDL3FilterEvent(uint32_t type)
    {
        EventHeader event {type};
        return eventFilter ? eventFilter(eventFilterData, &event) : true;
    }

    void FakeSDL3SetAsyncMainThread(bool async)
    {
        runCallbacksImmediately = !async;
    }

    void FakeSDL3RunMainThreadCallbacks()
    {
        auto callbacks = std::move(mainThreadCallbacks);
        mainThreadCallbacks.clear();
        for (const auto& [callback, data] : callbacks)
            callback(data);
    }
}
