#pragma once

#include <algorithm>
#include <functional>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace vkShade
{
    class EventBus
    {
    public:
        EventBus() = default;
        ~EventBus() = default;

        // Non-copyable, non-movable
        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;

        // Sink proxy for cleaner call site syntax
        template<typename EventType>
        class Sink
        {
        public:
            explicit Sink(EventBus& bus) : m_bus(bus) {}

            // Connect a free function
            template<void (*Func)(const EventType&)>
            void connect() { m_bus.connect_impl<EventType, Func>(); }

            // Connect a member function (method)
            template<auto Method, typename T>
            void connect(T* instance) { m_bus.connect_impl<EventType, T, Method>(instance); }

            // Disconnect a free function
            template<void (*Func)(const EventType&)>
            void disconnect() { m_bus.disconnect_impl<EventType, Func>(); }

            // Disconnect a member function (method)
            template<auto Method, typename T>
            void disconnect(T* instance) { m_bus.disconnect_impl<EventType, T, Method>(instance); }

        private:
            EventBus& m_bus;
        };

        template<typename EventType>
        Sink<EventType> sink()
        {
            return Sink<EventType>(*this);
        }

        // Publish an event (queues it for later processing)
        template<typename EventType>
        void enqueue(const EventType& event)
        {
            auto typeId = std::type_index(typeid(EventType));

            // Store a copy of the event
            m_eventQueue.push([typeId, event, this]()
            {
                auto it = m_subscribers.find(typeId);
                if (it != m_subscribers.end())
                {
                    for (const auto& [ptr, pInstance, callback] : it->second)
                        callback(&event, pInstance);
                }
            });
        }

        // Process all queued events
        void update()
        {
            while (!m_eventQueue.empty())
            {
                m_eventQueue.front()();
                m_eventQueue.pop();
            }
        }

        // Clear all queued events without processing them
        void clear()
        {
            while (!m_eventQueue.empty())
            {
                m_eventQueue.pop();
            }
        }

        // Clear all subscribers for a specific event type
        template<typename EventType>
        void disconnect_all()
        {
            auto typeId = std::type_index(typeid(EventType));
            m_subscribers.erase(typeId);
        }

    private:
        using Callback = std::function<void(const void*, void*)>;

        // Map: Type -> [(function pointer, instance pointer, callback)]
        std::unordered_map<std::type_index, std::vector<std::tuple<void*, void*, Callback>>> m_subscribers;

        // Queue of event dispatchers
        std::queue<std::function<void()>> m_eventQueue;

        // Connect a free function to an event type
        template<typename EventType, void (*Func)(const EventType&)>
        void connect_impl()
        {
            auto typeId = std::type_index(typeid(EventType));
            void* pFunction = reinterpret_cast<void*>(Func);

            // Check if already connected
            auto& handlers = m_subscribers[typeId];
            for (const auto& [ptr, pInstance, callback] : handlers)
            {
                if (ptr == pFunction && pInstance == nullptr)
                    return;  // Already connected
            }

            handlers.emplace_back(pFunction, nullptr, [](const void* pEvent, void*)
            {
                Func(*static_cast<const EventType*>(pEvent));
            });
        }

        // Connect a member function to an event type
        template<typename EventType, typename T, auto Method>
        void connect_impl(T* instance)
        {
            static_assert(std::is_same_v<decltype(Method), void (T::*)(const EventType&)>,
                "Handler must be a member function of the form: void (T::*)(const EventType&)");

            auto typeId = std::type_index(typeid(EventType));

            // Use union to convert member function pointer to void*
            union {
                void (T::*method)(const EventType&);
                void* ptr;
            } converter;
            converter.method = Method;
            void* pMethod = converter.ptr;

            // Check if already connected
            auto& handlers = m_subscribers[typeId];
            for (const auto& [ptr, pInstance, callback] : handlers)
            {
                if (ptr == pMethod && pInstance == instance)
                    return;  // Already connected
            }

            handlers.emplace_back(pMethod, instance, [](const void* pEvent, void* pInstance)
            {
                T* obj = static_cast<T*>(pInstance);
                (obj->*Method)(*static_cast<const EventType*>(pEvent));
            });
        }

        // Disconnect a free function from an event type
        template<typename EventType, void (*Func)(const EventType&)>
        void disconnect_impl()
        {
            auto typeId = std::type_index(typeid(EventType));
            void* pFunction = reinterpret_cast<void*>(Func);

            auto it = m_subscribers.find(typeId);
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
            }
        }

        // Disconnect a member function from an event type
        template<typename EventType, typename T, auto Method>
        void disconnect_impl(T* pInstance)
        {
            static_assert(std::is_same_v<decltype(Method), void (T::*)(const EventType&)>,
                "Handler must be a member function of the form: void (T::*)(const EventType&)");

            auto typeId = std::type_index(typeid(EventType));

            union {
                void (T::*method)(const EventType&);
                void* ptr;
            } converter;
            converter.method = Method;
            void* pMethod = converter.ptr;

            auto it = m_subscribers.find(typeId);
            if (it != m_subscribers.end())
            {
                auto& handlers = it->second;
                handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                    [pMethod, pInstance](const auto& tuple)
                    {
                        return std::get<0>(tuple) == pMethod && std::get<1>(tuple) == pInstance;
                    }),
                    handlers.end()
                );
            }
        }
    };
} // namespace vkShade
