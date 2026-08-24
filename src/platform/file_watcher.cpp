#include "file_watcher.hpp"

#if defined(__linux__)
    #include "linux/file_watcher.hpp"
    using FileWatcherImpl = vkShade::Platform::LinuxFileWatcher;
#else
    #error "Unsupported platform"
#endif

namespace vkShade::Platform
{
    std::unique_ptr<FileWatcher> FileWatcher::create()
    {
        try {
            return std::make_unique<FileWatcherImpl>();
        }
        catch (const std::system_error& e) {
            Logger::error("Failed to create file watcher: {}", e.what());
            return nullptr;
        }
    }
} // namespace vkShade::Platform
