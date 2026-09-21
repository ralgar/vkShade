#include "sdl_module.hpp"

#include <dlfcn.h>
#include <utility>

namespace
{
    const char* get_anchor_symbol(vkShade::SdlAbi abi)
    {
        switch (abi)
        {
            case vkShade::SdlAbi::Sdl2:
                return "SDL_GetRelativeMouseMode";
            case vkShade::SdlAbi::Sdl3:
                return "SDL_GetWindowRelativeMouseMode";
        }

        return nullptr;
    }
}

std::optional<vkShade::SdlModule> vkShade::SdlModule::find(SdlAbi abi)
{
    // An ABI-specific anchor identifies one concrete SDL image before shared
    // SDL2/SDL3 symbol names are resolved from it.
    void* anchor = dlsym(RTLD_DEFAULT, get_anchor_symbol(abi));
    if (!anchor)
        return std::nullopt;

    Dl_info info {};
    if (dladdr(anchor, &info) == 0 || !info.dli_fbase)
        return std::nullopt;

    void* handle = nullptr;
    std::string modulePath;
    if (info.dli_fname && *info.dli_fname)
    {
        modulePath = info.dli_fname;
        handle = dlopen(info.dli_fname, RTLD_NOW | RTLD_NOLOAD | RTLD_LOCAL);
    }
    else
    {
        modulePath = "<main program>";
        handle = dlopen(nullptr, RTLD_NOW | RTLD_LOCAL);
    }

    if (!handle)
        return std::nullopt;

    return SdlModule(handle, info.dli_fbase, std::move(modulePath));
}

vkShade::SdlModule::SdlModule(void* handle, void* baseAddress, std::string path)
    : m_handle(handle)
    , m_baseAddress(baseAddress)
    , m_path(std::move(path))
{
}

vkShade::SdlModule::SdlModule(SdlModule&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_baseAddress(std::exchange(other.m_baseAddress, nullptr))
    , m_path(std::move(other.m_path))
{
}

vkShade::SdlModule& vkShade::SdlModule::operator=(SdlModule&& other) noexcept
{
    if (this == &other)
        return *this;

    reset();
    m_handle = std::exchange(other.m_handle, nullptr);
    m_baseAddress = std::exchange(other.m_baseAddress, nullptr);
    m_path = std::move(other.m_path);
    return *this;
}

vkShade::SdlModule::~SdlModule()
{
    reset();
}

void* vkShade::SdlModule::get_symbol(const char* name) const
{
    if (!m_handle)
        return nullptr;

    void* result = dlsym(m_handle, name);
    if (!result)
        return nullptr;

    Dl_info info {};
    // A module handle may search dependencies too. Reject a same-named symbol
    // unless it belongs to the SDL image selected by the ABI anchor.
    if (dladdr(result, &info) == 0 || info.dli_fbase != m_baseAddress)
        return nullptr;

    return result;
}

const std::string& vkShade::SdlModule::get_path() const
{
    return m_path;
}

void vkShade::SdlModule::reset()
{
    if (m_handle)
        dlclose(m_handle);
    m_handle = nullptr;
    m_baseAddress = nullptr;
}
