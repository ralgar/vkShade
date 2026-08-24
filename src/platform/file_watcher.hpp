#pragma once

#include <filesystem>

namespace vkShade::Platform
{
    class FileWatcher
    {
    public:
        virtual ~FileWatcher() = default;

        virtual bool watch(const std::filesystem::path& path) = 0;
        virtual bool changed() = 0;

        static std::unique_ptr<FileWatcher> create();
    };
} // namespace vkShade::Platform
