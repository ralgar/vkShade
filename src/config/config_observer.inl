#pragma once

#include "config_observer.hpp"
#include "config_store.hpp"

// This file must only be included from config_store.hpp, AFTER
// class ConfigStore is fully defined. connect_impl's body calls
// ConfigStore::get<T>(), which requires the complete type.
//
// Do not include this file directly.

namespace vkShade
{
    // Connect free function
    template<auto Func>
    void ConfigObserver::connect_impl(const std::string& section, const std::string& key)
    {
        using FuncType = decltype(Func);
        using ArgType = std::decay_t<typename function_arg<FuncType>::type>;

        std::string mapKey = section + "::" + key;
        void* pFunction = reinterpret_cast<void*>(Func);

        // Check if already connected
        auto& handlers = m_subscribers[mapKey];
        if (std::ranges::any_of(handlers, [pFunction](const SubscriptionPtr& sub)
            {
                return sub->function == pFunction && sub->instance == nullptr;
            }))
            return;  // Already connected

        // If not, create a new subscription.
        auto subscription = std::make_shared<Subscription>();

        subscription->section = section;
        subscription->key = key;
        subscription->function = pFunction;
        subscription->instance = nullptr;

        subscription->callback = [](const void* pValue, void*, const std::string& k)
        {
            Func(k, *static_cast<const ArgType*>(pValue));
        };

        subscription->reload = [section, key](ConfigStore& store)
        {
            auto value = store.get<ArgType>(section, key);

            if (value)
                Func(key, *value);
        };

        handlers.push_back(std::move(subscription));
    }

    // Connect member function
    template<auto Method, typename Class>
    void ConfigObserver::connect_impl(const std::string& section, const std::string& key, Class* instance)
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
        if (std::ranges::any_of(handlers, [pMethod, instance](const SubscriptionPtr& sub)
            {
                return sub->function == pMethod && sub->instance == instance;
            }))
            return;  // Already connected

        // If not then create a new subscription
        auto subscription = std::make_shared<Subscription>();

        subscription->section = section;
        subscription->key = key;
        subscription->function = pMethod;
        subscription->instance = instance;

        subscription->callback = [](const void* pValue, void* pInstance, const std::string& k)
        {
            Class* obj = static_cast<Class*>(pInstance);
            (obj->*Method)(k, *static_cast<const ArgType*>(pValue));
        };

        subscription->reload = [instance, section, key](ConfigStore& store)
        {
            auto value = store.get<ArgType>(section, key);
            if (value)
                (instance->*Method)(key, *value);
        };

        handlers.push_back(std::move(subscription));
    }

    // Disconnect free function
    template<auto Func>
    void ConfigObserver::disconnect_impl(const std::string& section, const std::string& key)
    {
        std::string mapKey = section + "::" + key;
        void* pFunction = reinterpret_cast<void*>(Func);

        auto it = m_subscribers.find(mapKey);
        if (it != m_subscribers.end())
        {
            auto& handlers = it->second;
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                [pFunction](const SubscriptionPtr& sub)
                {
                    const bool matches = sub->function == pFunction && sub->instance == nullptr;
                    if (matches)
                        sub->connected = false;
                    return matches;
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
    void ConfigObserver::disconnect_impl(const std::string& section, const std::string& key, Class* instance)
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
                [pMethod, instance](const SubscriptionPtr& sub)
                {
                    const bool matches = sub->function == pMethod && sub->instance == instance;
                    if (matches)
                        sub->connected = false;
                    return matches;
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
} // namespace vkShade
