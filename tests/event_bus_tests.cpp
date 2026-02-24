#include "event_bus.hpp"

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace vkShade
{
    // Test event types
    struct TestEvent
    {
        int32_t value;
        std::string message;
    };

    struct AnotherEvent
    {
        double data;
    };

    struct CounterEvent
    {
        uint32_t count;
    };
} // namespace vkShade

using namespace vkShade;

// Global state for testing
static std::vector<std::string> g_eventLog;

// Free function handlers
void on_test_event_free(const TestEvent& event)
{
    g_eventLog.push_back("Free: " + std::to_string(event.value) + " - " + event.message);
}

void on_another_event_free(const AnotherEvent& event)
{
    g_eventLog.push_back("Free: " + std::to_string(event.data));
}

void on_counter_event_free(const CounterEvent& event)
{
    g_eventLog.push_back("Free: Count " + std::to_string(event.count));
}

// Test class with member function handlers
class EventReceiver
{
public:
    void on_test_event(const TestEvent& event)
    {
        g_eventLog.push_back("Member: " + std::to_string(event.value) + " - " + event.message);
        m_receivedCount++;
    }

    void on_another_event(const AnotherEvent& event)
    {
        g_eventLog.push_back("Member: " + std::to_string(event.data));
    }

    uint32_t get_received_count() const { return m_receivedCount; }

private:
    uint32_t m_receivedCount = 0;
};

TEST_CASE("Free function connection and event dispatch", "[eventbus][basic]")
{
    g_eventLog.clear();
    EventBus bus;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.enqueue(TestEvent{42, "Hello"});
    bus.update();

    REQUIRE(g_eventLog.size() == 1);
    REQUIRE(g_eventLog[0] == "Free: 42 - Hello");
}

TEST_CASE("Member function connection and event dispatch", "[eventbus][basic]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver;

    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver);
    bus.enqueue(TestEvent{100, "World"});
    bus.update();

    REQUIRE(g_eventLog.size() == 1);
    REQUIRE(g_eventLog[0] == "Member: 100 - World");
    REQUIRE(receiver.get_received_count() == 1);
}

TEST_CASE("Multiple handlers for same event", "[eventbus][handlers]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver);
    bus.enqueue(TestEvent{50, "Multiple"});
    bus.update();

    REQUIRE(g_eventLog.size() == 2);
    REQUIRE(g_eventLog[0] == "Free: 50 - Multiple");
    REQUIRE(g_eventLog[1] == "Member: 50 - Multiple");
}

TEST_CASE("Multiple events queued and processed in order", "[eventbus][queue]")
{
    g_eventLog.clear();
    EventBus bus;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.sink<CounterEvent>().connect<&on_counter_event_free>();

    bus.enqueue(TestEvent{1, "First"});
    bus.enqueue(TestEvent{2, "Second"});
    bus.enqueue(CounterEvent{99});
    bus.enqueue(TestEvent{3, "Third"});

    bus.update();

    REQUIRE(g_eventLog.size() == 4);
    REQUIRE(g_eventLog[0] == "Free: 1 - First");
    REQUIRE(g_eventLog[1] == "Free: 2 - Second");
    REQUIRE(g_eventLog[2] == "Free: Count 99");
    REQUIRE(g_eventLog[3] == "Free: 3 - Third");
}

TEST_CASE("Disconnect free function", "[eventbus][disconnect]")
{
    g_eventLog.clear();
    EventBus bus;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.enqueue(TestEvent{1, "Before disconnect"});
    bus.update();

    bus.sink<TestEvent>().disconnect<&on_test_event_free>();
    bus.enqueue(TestEvent{2, "After disconnect"});
    bus.update();

    REQUIRE(g_eventLog.size() == 1);
    REQUIRE(g_eventLog[0] == "Free: 1 - Before disconnect");
}

TEST_CASE("Disconnect member function", "[eventbus][disconnect]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver;

    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver);
    bus.enqueue(TestEvent{1, "Before disconnect"});
    bus.update();

    bus.sink<TestEvent>().disconnect<&EventReceiver::on_test_event>(&receiver);
    bus.enqueue(TestEvent{2, "After disconnect"});
    bus.update();

    REQUIRE(g_eventLog.size() == 1);
    REQUIRE(g_eventLog[0] == "Member: 1 - Before disconnect");
    REQUIRE(receiver.get_received_count() == 1);
}

TEST_CASE("Disconnect all handlers for event type", "[eventbus][disconnect]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver);
    bus.enqueue(TestEvent{1, "Before disconnect all"});
    bus.update();

    bus.disconnect_all<TestEvent>();
    bus.enqueue(TestEvent{2, "After disconnect all"});
    bus.update();

    REQUIRE(g_eventLog.size() == 2);  // Only first event processed
}

TEST_CASE("Clear event queue without processing", "[eventbus][queue]")
{
    g_eventLog.clear();
    EventBus bus;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.enqueue(TestEvent{1, "First"});
    bus.enqueue(TestEvent{2, "Second"});
    bus.enqueue(TestEvent{3, "Third"});

    bus.clear();
    bus.update();

    REQUIRE(g_eventLog.empty());
}

TEST_CASE("Multiple instances with same handler", "[eventbus][handlers]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver1;
    EventReceiver receiver2;

    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver1);
    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver2);
    bus.enqueue(TestEvent{77, "Shared"});
    bus.update();

    REQUIRE(g_eventLog.size() == 2);
    REQUIRE(receiver1.get_received_count() == 1);
    REQUIRE(receiver2.get_received_count() == 1);
}

TEST_CASE("Disconnect specific instance only", "[eventbus][disconnect]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver1;
    EventReceiver receiver2;

    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver1);
    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver2);

    bus.sink<TestEvent>().disconnect<&EventReceiver::on_test_event>(&receiver1);

    bus.enqueue(TestEvent{88, "After disconnect one"});
    bus.update();

    REQUIRE(g_eventLog.size() == 1);
    REQUIRE(g_eventLog[0] == "Member: 88 - After disconnect one");
    REQUIRE(receiver1.get_received_count() == 0);
    REQUIRE(receiver2.get_received_count() == 1);
}

TEST_CASE("Prevent duplicate connections", "[eventbus][handlers]")
{
    g_eventLog.clear();
    EventBus bus;
    EventReceiver receiver;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.sink<TestEvent>().connect<&on_test_event_free>();  // Should be ignored
    bus.sink<TestEvent>().connect<&on_test_event_free>();  // Should be ignored

    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver);
    bus.sink<TestEvent>().connect<&EventReceiver::on_test_event>(&receiver);  // Should be ignored

    bus.enqueue(TestEvent{1, "Duplicate test"});
    bus.update();

    REQUIRE(g_eventLog.size() == 2);  // Only 2 handlers, not 5
}

TEST_CASE("Different event types don't interfere", "[eventbus][isolation]")
{
    g_eventLog.clear();
    EventBus bus;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.sink<AnotherEvent>().connect<&on_another_event_free>();

    bus.enqueue(TestEvent{1, "Test"});
    bus.enqueue(AnotherEvent{3.14});
    bus.enqueue(TestEvent{2, "Test2"});
    bus.update();

    REQUIRE(g_eventLog.size() == 3);
    REQUIRE(g_eventLog[0] == "Free: 1 - Test");
    REQUIRE(g_eventLog[1] == "Free: 3.140000");
    REQUIRE(g_eventLog[2] == "Free: 2 - Test2");
}

TEST_CASE("Events remain queued until update is called", "[eventbus][queue]")
{
    g_eventLog.clear();
    EventBus bus;

    bus.sink<TestEvent>().connect<&on_test_event_free>();
    bus.enqueue(TestEvent{1, "First"});
    bus.enqueue(TestEvent{2, "Second"});

    REQUIRE(g_eventLog.empty());  // Nothing processed yet

    bus.update();

    REQUIRE(g_eventLog.size() == 2);
}
