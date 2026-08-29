#pragma once

#include <optional>
#include <string>

namespace vkShade
{
    enum class SdlAbi
    {
        Sdl2,
        Sdl3,
    };

    class SdlModule
    {
    public:
        static std::optional<SdlModule> find(SdlAbi abi);

        SdlModule(const SdlModule&) = delete;
        SdlModule& operator=(const SdlModule&) = delete;
        SdlModule(SdlModule&& other) noexcept;
        SdlModule& operator=(SdlModule&& other) noexcept;
        ~SdlModule();

        template<typename Function>
        Function get_function(const char* name) const
        {
            return reinterpret_cast<Function>(get_symbol(name));
        }

        const std::string& get_path() const;

    private:
        SdlModule(void* handle, void* baseAddress, std::string path);

        void* get_symbol(const char* name) const;
        void reset();

        void* m_handle {nullptr};
        void* m_baseAddress {nullptr};
        std::string m_path;
    };
} // namespace vkShade
