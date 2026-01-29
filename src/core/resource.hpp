#pragma once

#include <atomic>
#include <type_traits>

namespace vkShade
{
    // Abstract base class for all resource types.
    // Derived classes implement load() to initialize resources from stored parameters.
    // Thread-safe ready flag allows checking if resource has finished loading.
    class Resource
    {
    public:
        virtual ~Resource() = default;

        // Resources use unique ownership semantics
        // NOTE: We implement move because std::atomic lacks it
        Resource(const Resource&) = delete;
        Resource& operator=(const Resource&) = delete;

        Resource(Resource&& other) noexcept
            : m_ready(other.m_ready.load(std::memory_order_relaxed))
        {}

        Resource& operator=(Resource&& other) noexcept
        {
            if (this != &other)
                m_ready.store(other.m_ready.load(std::memory_order_relaxed), std::memory_order_relaxed);
            return *this;
        }

        virtual bool load() = 0;

        bool is_valid() const { return m_valid; }
        bool is_ready() const { return m_ready.load(std::memory_order_acquire); }

    protected:
        // Prevent direct instantiation
        Resource() = default;

        bool m_valid {true};
        std::atomic<bool> m_ready {false};

        void set_ready(bool value) { m_ready.store(value, std::memory_order_release); }
    };

    // Constraint for types derived from Resource
    template<typename T>
    concept IsResource = std::is_base_of_v<Resource, T>;
} // namespace vkShade
