#pragma once

#include <memory>
#include <stdexcept>

namespace vkShade
{
    template<typename T>
    class Locator
    {
    public:
        template<typename Derived = T, typename... Args>
        static T& emplace(Args&&... args)
        {
            static_assert(std::is_base_of_v<T, Derived> || std::is_same_v<T, Derived>,
                         "Derived must inherit from T or be T");

            if (!m_instance)
            {
                m_instance = std::make_unique<Derived>(std::forward<Args>(args)...);
            }

            return *m_instance;
        }

        static T& get()
        {
            if (!m_instance)
            {
                throw std::runtime_error("Service not registered");
            }

            return *m_instance;
        }

        static bool has()
        {
            return m_instance != nullptr;
        }

        static void reset()
        {
            m_instance.reset();
        }

    private:
        static std::unique_ptr<T> m_instance;
    };

    // Define static member
    template<typename T>
    std::unique_ptr<T> Locator<T>::m_instance;
} // namespace vkShade
