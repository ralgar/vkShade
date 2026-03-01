#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include "resource.hpp"
#include "resource_factory.hpp"

namespace vkShade
{
    /// Thread-safe cache for resources of type T with automatic lifetime management.
    /// Resources are stored as weak_ptr to allow automatic unloading when no longer referenced.
    /// Uses reader-writer locks for efficient concurrent access during reads.
    template<IsResource T>
    class ResourceCache
    {
    public:
        ResourceCache() { load_default_resources(); }

        // Load a new resource, or get from the cache if already loaded.
        template<typename... Args>
        std::shared_ptr<T> load(const std::string& cacheKey, Args&&... args)
        {
            // Check if resource is already loaded
            if (auto resource = try_get(cacheKey))
                return resource;

            spdlog::debug("Loading resource: {}", cacheKey);
            std::shared_ptr<T> resource;

            {
                std::unique_lock lock(m_mutex);  // Write lock
                resource = ResourceFactory<T>::create(std::forward<Args>(args)...);
                if (resource->is_valid())  // Only cache if valid
                    m_cache[cacheKey] = resource;
            }

            if (!resource || !resource->is_valid())
            {
                spdlog::error("Failed to create resource: {}", cacheKey);
                return nullptr;
            }

            // Actually load the resource (outside of lock due to expense)
            if (!resource->load())
            {
                spdlog::error("Failed to load resource: {}", cacheKey);
                std::unique_lock lock(m_mutex);  // Write lock
                m_cache.erase(cacheKey);
                return nullptr;
            }

            return resource;
        }

        // Try to get a resource from cache, and fail if not already loaded.
        std::shared_ptr<T> try_get(const std::string& cacheKey) const
        {
            std::shared_lock lock(m_mutex);  // Read lock

            auto it = m_cache.find(cacheKey);
            if (it != m_cache.end())
            {
                // Upgrade weak_ptr to shared_ptr
                if (auto resource = it->second.lock())
                    return std::dynamic_pointer_cast<T>(resource);
            }

            // Not found or expired
            return nullptr;
        }

    private:
        // Using a weak_ptr here is non-owning and allows automatic unloading.
        std::unordered_map<std::string, std::weak_ptr<T>> m_cache;

        // Store default/owned resources.
        std::vector<std::shared_ptr<T>> m_defaultResources;

        // Reader/writer lock.
        mutable std::shared_mutex m_mutex;

        // Optional initialization hook. Does nothing by default.
        // cppcheck-suppress functionStatic
        void load_default_resources() {}
    };
} // namespace vkShade
