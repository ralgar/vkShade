#include <catch2/catch_test_macros.hpp>

#include <dlfcn.h>

#include "input/sdl_module.hpp"

namespace
{
    class DynamicLibrary
    {
    public:
        explicit DynamicLibrary(const char* path)
            : m_handle(dlopen(path, RTLD_NOW | RTLD_GLOBAL))
        {
            REQUIRE(m_handle != nullptr);
        }

        ~DynamicLibrary()
        {
            if (m_handle)
                dlclose(m_handle);
        }

        void* symbol(const char* name) const
        {
            return dlsym(m_handle, name);
        }

    private:
        void* m_handle {nullptr};
    };
}

TEST_CASE("SDL modules keep shared symbols bound to their detected ABI")
{
    const DynamicLibrary sdl3(FAKE_SDL3_PATH);
    const DynamicLibrary sdl2(FAKE_SDL2_PATH);

    REQUIRE(dlsym(RTLD_DEFAULT, "SDL_GetEventFilter")
            == sdl3.symbol("SDL_GetEventFilter"));

    const auto module2 = vkShade::SdlModule::find(vkShade::SdlAbi::Sdl2);
    const auto module3 = vkShade::SdlModule::find(vkShade::SdlAbi::Sdl3);

    REQUIRE(module2.has_value());
    REQUIRE(module3.has_value());
    CHECK(module2->get_function<void*>("SDL_GetEventFilter")
          == sdl2.symbol("SDL_GetEventFilter"));
    CHECK(module3->get_function<void*>("SDL_GetEventFilter")
          == sdl3.symbol("SDL_GetEventFilter"));
    CHECK(module2->get_function<void*>("SDL_SetEventFilter")
          == sdl2.symbol("SDL_SetEventFilter"));
    CHECK(module3->get_function<void*>("SDL_SetEventFilter")
          == sdl3.symbol("SDL_SetEventFilter"));
}
