#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkShade
{
    class ConfigObserver
    {
    public:
        class Sink
        {
        public:
            Sink(ConfigObserver& observer, const std::string& section, const std::string& key)
                : m_observer(observer), m_section(section), m_key(key) {}

            // Connect a free function
            template<auto Func>
            void connect() { m_observer.connect_impl<Func>(m_section, m_key); }

            // Connect a member function (method)
            template<auto Method, typename Class>
            void connect(Class* instance) { m_observer.connect_impl<Method>(m_section, m_key, instance); }

            // Disconnect a free function
            template<auto Func>
            void disconnect() { m_observer.disconnect_impl<Func>(m_section, m_key); }

            // Disconnect a member function (method)
            template<auto Method, typename Class>
            void disconnect(Class* instance) { m_observer.disconnect_impl<Method>(m_section, m_key, instance); }

        private:
            ConfigObserver& m_observer;
            std::string     m_section;
            std::string     m_key;
        };

        Sink on_changed(const std::string& section, const std::string& key)
        {
            return Sink(*this, section, key);
        }

        template<typename T>
        void notify(const std::string& section, const std::string& key, const T& value)
        {
            std::string mapKey = section + "::" + key;
            auto it = m_subscribers.find(mapKey);
            if (it != m_subscribers.end())
            {
                for (const auto& [ptr, pInstance, callback] : it->second)
                {
                    callback(&value, pInstance);
                }
            }
        }

    private:
        using Callback = std::function<void(const void*, void*)>;

        // Map: "Section::Key" -> [(function pointer, instance pointer, callback)]
        std::unordered_map<std::string, std::vector<std::tuple<void*, void*, Callback>>> m_subscribers;

        // Helper traits
        template<typename T>
        struct function_arg;

        template<typename Ret, typename Arg>
        struct function_arg<Ret(*)(Arg)>
        {
            using type = Arg;
        };

        template<typename T>
        struct method_arg;

        template<typename Class, typename Ret, typename Arg>
        struct method_arg<Ret(Class::*)(Arg)>
        {
            using type = Arg;
        };

        // Connect free function
        template<auto Func>
        void connect_impl(const std::string& section, const std::string& key)
        {
            using FuncType = decltype(Func);
            using ArgType = std::decay_t<typename function_arg<FuncType>::type>;

            std::string mapKey = section + "::" + key;
            void* pFunction = reinterpret_cast<void*>(Func);

            // Check if already connected
            auto& handlers = m_subscribers[mapKey];
            for (const auto& [ptr, pInstance, callback] : handlers)
            {
                if (ptr == pFunction && pInstance == nullptr)
                    return;  // Already connected
            }

            handlers.emplace_back(pFunction, nullptr, [](const void* pValue, void*)
            {
                Func(*static_cast<const ArgType*>(pValue));
            });
        }

        // Connect member function
        template<auto Method, typename Class>
        void connect_impl(const std::string& section, const std::string& key, Class* instance)
        {
            using MethodType = decltype(Method);
            using ArgType = std::decay_t<typename method_arg<MethodType>::type>;

            std::string mapKey = section + "::" + key;

            union {
                MethodType method;
                void* ptr;
            } converter;
            converter.method = Method;
            void* pMethod = converter.ptr;

            // Check if already connected
            auto& handlers = m_subscribers[mapKey];
            for (const auto& [ptr, pInstance, callback] : handlers)
            {
                if (ptr == pMethod && pInstance == instance)
                    return;  // Already connected
            }

            handlers.emplace_back(pMethod, instance, [](const void* pValue, void* pInstance)
            {
                Class* obj = static_cast<Class*>(pInstance);
                (obj->*Method)(*static_cast<const ArgType*>(pValue));
            });
        }

        // Disconnect free function
        template<auto Func>
        void disconnect_impl(const std::string& section, const std::string& key)
        {
            std::string mapKey = section + "::" + key;
            void* pFunction = reinterpret_cast<void*>(Func);

            auto it = m_subscribers.find(mapKey);
            if (it != m_subscribers.end())
            {
                auto& handlers = it->second;
                handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                    [pFunction](const auto& tuple)
                    {
                        return std::get<0>(tuple) == pFunction && std::get<1>(tuple) == nullptr;
                    }),
                    handlers.end()
                );

                // Clean up empty entries
                if (handlers.empty())
                {
                    m_subscribers.erase(it);
                }
            }
        }

        // Disconnect member function
        template<auto Method, typename Class>
        void disconnect_impl(const std::string& section, const std::string& key, Class* instance)
        {
            std::string mapKey = section + "::" + key;

            union {
                decltype(Method) method;
                void* ptr;
            } converter;
            converter.method = Method;
            void* pMethod = converter.ptr;

            auto it = m_subscribers.find(mapKey);
            if (it != m_subscribers.end())
            {
                auto& handlers = it->second;
                handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                    [pMethod, instance](const auto& tuple)
                    {
                        return std::get<0>(tuple) == pMethod && std::get<1>(tuple) == instance;
                    }),
                    handlers.end()
                );

                // Clean up empty entries
                if (handlers.empty())
                {
                    m_subscribers.erase(it);
                }
            }
        }
    };
} // namespace vkShade
