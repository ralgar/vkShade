#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkShade
{
    class ConfigStore;

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
                for (const auto& subscription : it->second)
                {
                    subscription.callback(&value, subscription.instance, key);
                }
            }
        }

        void notify_all(ConfigStore& store)
        {
            for (auto& [_, subscriptions] : m_subscribers)
                for (auto& subscription : subscriptions)
                    subscription.reload(store);
        }

    private:
        using Callback = std::function<void(const void*, void*, const std::string&)>;

        struct Subscription
        {
            std::string section;
            std::string key;

            void* function;
            void* instance;

            Callback callback;
            std::function<void(ConfigStore&)> reload;
        };

        // Map: "Section::Key" -> [Subscription]
        std::unordered_map<std::string, std::vector<Subscription>> m_subscribers;

        // Helper traits
        template<typename T>
        struct function_arg;

        template<typename Ret, typename Arg>
        struct function_arg<Ret(*)(const std::string&, Arg)>
        {
            using type = Arg;
        };

        template<typename T>
        struct method_arg;

        template<typename Class, typename Ret, typename Arg>
        struct method_arg<Ret(Class::*)(const std::string&, Arg)>
        {
            using type = Arg;
        };

        // Connect free function
        template<auto Func>
        void connect_impl(const std::string& section, const std::string& key);

        // Connect member function
        template<auto Method, typename Class>
        void connect_impl(const std::string& section, const std::string& key, Class* instance);

        // Disconnect free function
        template<auto Func>
        void disconnect_impl(const std::string& section, const std::string& key);

        // Disconnect member function
        template<auto Method, typename Class>
        void disconnect_impl(const std::string& section, const std::string& key, Class* instance);
    };
} // namespace vkShade
