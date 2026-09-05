#include <cstdint>

extern "C"
{
    int SDL_GetRelativeMouseMode()
    {
        return 0;
    }

    int SDL_GetEventFilter(void*, void**)
    {
        return 0;
    }

    void SDL_SetEventFilter(void*, void*)
    {
    }

    void SDL_FlushEvents(uint32_t, uint32_t)
    {
    }
}
