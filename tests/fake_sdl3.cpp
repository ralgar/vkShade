#include <cstdint>

extern "C"
{
    bool SDL_GetWindowRelativeMouseMode(void*)
    {
        return false;
    }

    bool SDL_GetEventFilter(void*, void**)
    {
        return false;
    }

    void SDL_SetEventFilter(void*, void*)
    {
    }

    void SDL_FlushEvents(uint32_t, uint32_t)
    {
    }
}
