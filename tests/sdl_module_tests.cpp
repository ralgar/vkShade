#include <catch2/catch_test_macros.hpp>

#include <dlfcn.h>

#include "input/application_mouse_inhibitors.hpp"
#include "input/mouse_capture_controller.hpp"
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

        template<typename Function>
        Function function(const char* name) const
        {
            return reinterpret_cast<Function>(symbol(name));
        }

    private:
        void* m_handle {nullptr};
    };
}

TEST_CASE("SDL3 inhibition uses its window and callback ABI")
{
    const DynamicLibrary sdl3(FAKE_SDL3_PATH);
    const DynamicLibrary sdl2(FAKE_SDL2_PATH);

    const auto reset = sdl3.function<void (*)()>("FakeSDL3Reset");
    const auto relativeMode =
        sdl3.function<bool (*)()>("FakeSDL3RelativeMouseMode");
    const auto filterEvent =
        sdl3.function<bool (*)(uint32_t)>("FakeSDL3FilterEvent");
    REQUIRE(reset);
    REQUIRE(relativeMode);
    REQUIRE(filterEvent);

    reset();
    auto inhibitor = vkShade::create_application_mouse_inhibitor();
    REQUIRE(inhibitor->inhibit());
    CHECK_FALSE(relativeMode());
    CHECK_FALSE(filterEvent(0x400));
    CHECK(filterEvent(0x300));

    inhibitor->restore();
    CHECK(relativeMode());
    CHECK(filterEvent(0x400));
}

TEST_CASE("SDL3 inhibition ignores stale main-thread work")
{
    const DynamicLibrary sdl3(FAKE_SDL3_PATH);
    const DynamicLibrary sdl2(FAKE_SDL2_PATH);

    const auto reset = sdl3.function<void (*)()>("FakeSDL3Reset");
    const auto setAsync =
        sdl3.function<void (*)(bool)>("FakeSDL3SetAsyncMainThread");
    const auto runCallbacks =
        sdl3.function<void (*)()>("FakeSDL3RunMainThreadCallbacks");
    const auto relativeMode =
        sdl3.function<bool (*)()>("FakeSDL3RelativeMouseMode");
    REQUIRE(reset);
    REQUIRE(setAsync);
    REQUIRE(runCallbacks);
    REQUIRE(relativeMode);

    reset();
    setAsync(true);
    auto inhibitor = vkShade::create_application_mouse_inhibitor();

    REQUIRE(inhibitor->inhibit());
    inhibitor->restore();
    runCallbacks();
    CHECK(relativeMode());

    REQUIRE(inhibitor->inhibit());
    runCallbacks();
    CHECK_FALSE(relativeMode());

    inhibitor->restore();
    REQUIRE(inhibitor->inhibit());
    runCallbacks();
    CHECK_FALSE(relativeMode());

    inhibitor->restore();
    runCallbacks();
    CHECK(relativeMode());
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
