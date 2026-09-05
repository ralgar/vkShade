#include "platform/file_watcher.hpp"

#include <cstring>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>

#include "core/logger.hpp"

namespace vkShade::Platform
{
    class LinuxFileWatcher final : public FileWatcher
    {
    public:
        LinuxFileWatcher()
        {
            m_fd = inotify_init1(IN_NONBLOCK);
            if (m_fd < 0)
                throw std::system_error(errno, std::generic_category(), "inotify_init1");
        }

        ~LinuxFileWatcher() override
        {
            if (m_fd >= 0)
                close(m_fd);
        }

        bool watch(const std::filesystem::path& path) override
        {
            // Remove any existing watch first
            this->unwatch();

            m_path = std::filesystem::absolute(path);

            m_watchDescriptor = inotify_add_watch(m_fd, m_path.parent_path().c_str(),
                IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_DELETE_SELF);

            if (m_watchDescriptor >= 0)
                Logger::debug("Watching for changes: {}", m_path.string());

            return m_watchDescriptor >= 0;
        }

        void unwatch() override
        {
            if (m_watchDescriptor >= 0)
            {
                if (inotify_rm_watch(m_fd, m_watchDescriptor) != 0)
                    Logger::trace("inotify_rm_watch() failed (errno {}), descriptor may already be invalid", errno);

                Logger::debug("Stopped watching: {}", m_path.string());
                m_watchDescriptor = -1;
                m_path.clear();
            }
        }

        bool changed() override
        {
            if (m_fd < 0 || m_watchDescriptor < 0)
                return false;

            // The buffer used for reading from the inotify file descriptor
            //  should have the same alignment as struct inotify_event.
            alignas(inotify_event) char buffer[4096];

            bool changed = false;
            while (true)
            {
                ssize_t bytesRead = read(m_fd, buffer, sizeof(buffer));

                if (bytesRead < 0)
                {
                    if (errno == EINTR)
                        continue;   // Interrupted by a signal. Retry the read.
                    if (errno == EAGAIN)
                        break;      // Non-blocking fd has no more events to read.

                    // Any other error means the inotify fd can no longer be used.
                    Logger::error("inotify read failed: {}", std::strerror(errno));
                    close(m_fd);
                    m_fd = -1;
                    m_watchDescriptor = -1;
                    break;
                }

                if (bytesRead == 0)
                    break;

                size_t offset = 0;
                while (offset < static_cast<size_t>(bytesRead))
                {
                    const auto* event = reinterpret_cast<const inotify_event*>(buffer + offset);

                    if (event->wd == m_watchDescriptor)
                    {
                        if (event->mask & IN_IGNORED)
                        {
                            // The watch was invalidated, e.g. because the watched
                            //  directory was deleted or unmounted.
                            Logger::error("inotify watch invalidated: {}", m_path.parent_path().string());
                            m_watchDescriptor = -1;
                            changed = false;
                        }
                        else if (event->len > 0 && m_path.filename() == event->name)
                        {
                            // Treat writes, atomic replacements, creation, and deletion as changes.
                            // Editors commonly replace files by renaming a temporary file over them.
                            if (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE))
                            {
                                Logger::debug("Change detected: {}", m_path.string());
                                changed = true;
                            }
                        }
                    }

                    offset += sizeof(inotify_event) + event->len;
                }
            }

            return changed;
        }

    private:
        int m_fd {-1};
        int m_watchDescriptor {-1};

        std::filesystem::path m_path;
    };
} // namespace vkShade::Platform
