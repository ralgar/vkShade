#include "config/config_observer.hpp"
#include "config/config_store.hpp"

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

using namespace vkShade;

// Global state for testing
static std::vector<std::string> g_callbackLog;

// Free function handlers
void on_string_changed(const std::string& key, const std::string& value)
{
    g_callbackLog.push_back("string:" + value);
}

void on_float_changed(const std::string& key, float value)
{
    g_callbackLog.push_back("float:" + std::to_string(value));
}

void on_bool_changed(const std::string& key, bool value)
{
    g_callbackLog.push_back("bool:" + std::string(value ? "true" : "false"));
}

void on_vec3_changed(const std::string& key, const glm::vec3& value)
{
    g_callbackLog.push_back("vec3:" + std::to_string(value.x) + "," +
                           std::to_string(value.y) + "," + std::to_string(value.z));
}

// Test class with member function handlers
class TestListener
{
public:
    void on_string_value(const std::string& key, const std::string& value)
    {
        m_lastString = value;
        m_callCount++;
    }

    void on_float_value(const std::string& key, float value)
    {
        m_lastFloat = value;
        m_callCount++;
    }

    void on_int_value(const std::string& key, int32_t value)
    {
        m_lastInt = value;
        m_callCount++;
    }

    std::string get_last_string() const { return m_lastString; }
    float get_last_float() const { return m_lastFloat; }
    int32_t get_last_int() const { return m_lastInt; }
    uint32_t get_call_count() const { return m_callCount; }

private:
    std::string m_lastString;
    float m_lastFloat = 0.0f;
    int32_t m_lastInt = 0;
    uint32_t m_callCount = 0;
};

class ObserverReplacementListener
{
public:
    void on_value(const std::string&, const std::string&)
    {
        callCount++;
    }

    uint32_t callCount {0};
};

class ObserverMutationListener
{
public:
    ObserverMutationListener(ConfigObserver& observer,
                             ObserverReplacementListener& removed,
                             ObserverReplacementListener& replacement)
        : m_observer(observer),
          m_removed(removed),
          m_replacement(replacement)
    {
    }

    void on_value(const std::string&, const std::string&)
    {
        callCount++;
        m_observer.on_changed("test", "value")
            .disconnect<&ObserverReplacementListener::on_value>(&m_removed);
        m_observer.on_changed("test", "value")
            .connect<&ObserverReplacementListener::on_value>(&m_replacement);
    }

    uint32_t callCount {0};

private:
    ConfigObserver& m_observer;
    ObserverReplacementListener& m_removed;
    ObserverReplacementListener& m_replacement;
};

TEST_CASE("ConfigObserver: Connect free function and notify", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("graphics", "quality").connect<&on_string_changed>();
    observer.notify("graphics", "quality", std::string("high"));

    REQUIRE(g_callbackLog.size() == 1);
    REQUIRE(g_callbackLog[0] == "string:high");
}

TEST_CASE("ConfigObserver: Connect member function and notify", "[config][observer]")
{
    TestListener listener;
    ConfigObserver observer;

    observer.on_changed("audio", "volume").connect<&TestListener::on_float_value>(&listener);
    observer.notify("audio", "volume", 0.75f);

    REQUIRE(listener.get_call_count() == 1);
    REQUIRE(listener.get_last_float() == 0.75f);
}

TEST_CASE("ConfigObserver: Multiple handlers for same key", "[config][observer]")
{
    g_callbackLog.clear();
    TestListener listener;
    ConfigObserver observer;

    observer.on_changed("test", "value").connect<&on_string_changed>();
    observer.on_changed("test", "value").connect<&TestListener::on_string_value>(&listener);
    observer.notify("test", "value", std::string("hello"));

    REQUIRE(g_callbackLog.size() == 1);
    REQUIRE(g_callbackLog[0] == "string:hello");
    REQUIRE(listener.get_call_count() == 1);
    REQUIRE(listener.get_last_string() == "hello");
}

TEST_CASE("ConfigObserver: Subscription changes take effect on the next notification",
          "[config][observer]")
{
    ConfigObserver observer;
    ObserverReplacementListener removed;
    ObserverReplacementListener replacement;
    ObserverMutationListener mutation(observer, removed, replacement);

    observer.on_changed("test", "value")
        .connect<&ObserverMutationListener::on_value>(&mutation);
    observer.on_changed("test", "value")
        .connect<&ObserverReplacementListener::on_value>(&removed);

    observer.notify("test", "value", std::string("first"));
    CHECK(mutation.callCount == 1);
    CHECK(removed.callCount == 0);
    CHECK(replacement.callCount == 0);

    observer.notify("test", "value", std::string("second"));
    CHECK(mutation.callCount == 2);
    CHECK(removed.callCount == 0);
    CHECK(replacement.callCount == 1);
}

TEST_CASE("ConfigObserver: Different sections and keys don't interfere", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("graphics", "quality").connect<&on_string_changed>();
    observer.on_changed("audio", "quality").connect<&on_float_changed>();

    observer.notify("graphics", "quality", std::string("high"));
    observer.notify("audio", "quality", 5.0f);

    REQUIRE(g_callbackLog.size() == 2);
    REQUIRE(g_callbackLog[0] == "string:high");
    REQUIRE(g_callbackLog[1] == "float:5.000000");
}

TEST_CASE("ConfigObserver: Notify with no handlers does nothing", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.notify("nonexistent", "key", std::string("value"));

    REQUIRE(g_callbackLog.empty());
}

TEST_CASE("ConfigObserver: Prevent duplicate connections - free function", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&on_string_changed>();
    observer.on_changed("test", "key").connect<&on_string_changed>();
    observer.on_changed("test", "key").connect<&on_string_changed>();

    observer.notify("test", "key", std::string("once"));

    REQUIRE(g_callbackLog.size() == 1);
}

TEST_CASE("ConfigObserver: Prevent duplicate connections - member function", "[config][observer]")
{
    TestListener listener;
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&TestListener::on_string_value>(&listener);
    observer.on_changed("test", "key").connect<&TestListener::on_string_value>(&listener);

    observer.notify("test", "key", std::string("once"));

    REQUIRE(listener.get_call_count() == 1);
}

TEST_CASE("ConfigObserver: Multiple instances with same handler", "[config][observer]")
{
    TestListener listener1;
    TestListener listener2;
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&TestListener::on_int_value>(&listener1);
    observer.on_changed("test", "key").connect<&TestListener::on_int_value>(&listener2);

    observer.notify("test", "key", 42);

    REQUIRE(listener1.get_call_count() == 1);
    REQUIRE(listener1.get_last_int() == 42);
    REQUIRE(listener2.get_call_count() == 1);
    REQUIRE(listener2.get_last_int() == 42);
}

TEST_CASE("ConfigObserver: Notify different types", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("test", "bool").connect<&on_bool_changed>();
    observer.on_changed("test", "float").connect<&on_float_changed>();
    observer.on_changed("test", "vec3").connect<&on_vec3_changed>();

    observer.notify("test", "bool", true);
    observer.notify("test", "float", 3.14f);
    observer.notify("test", "vec3", glm::vec3(1.0f, 2.0f, 3.0f));

    REQUIRE(g_callbackLog.size() == 3);
    REQUIRE(g_callbackLog[0] == "bool:true");
    REQUIRE(g_callbackLog[1] == "float:3.140000");
}

TEST_CASE("ConfigObserver: Same key in different sections are independent", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("section1", "value").connect<&on_string_changed>();
    observer.on_changed("section2", "value").connect<&on_float_changed>();

    observer.notify("section1", "value", std::string("test"));

    REQUIRE(g_callbackLog.size() == 1);
    REQUIRE(g_callbackLog[0] == "string:test");
}

TEST_CASE("ConfigObserver: Multiple notifications trigger handler each time", "[config][observer]")
{
    TestListener listener;
    ConfigObserver observer;

    observer.on_changed("test", "counter").connect<&TestListener::on_int_value>(&listener);

    observer.notify("test", "counter", 1);
    observer.notify("test", "counter", 2);
    observer.notify("test", "counter", 3);

    REQUIRE(listener.get_call_count() == 3);
    REQUIRE(listener.get_last_int() == 3);
}

TEST_CASE("ConfigObserver: Disconnect free function", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&on_string_changed>();
    observer.notify("test", "key", std::string("first"));

    observer.on_changed("test", "key").disconnect<&on_string_changed>();
    observer.notify("test", "key", std::string("second"));

    REQUIRE(g_callbackLog.size() == 1);
    REQUIRE(g_callbackLog[0] == "string:first");
}

TEST_CASE("ConfigObserver: Disconnect member function", "[config][observer]")
{
    TestListener listener;
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&TestListener::on_int_value>(&listener);
    observer.notify("test", "key", 10);

    observer.on_changed("test", "key").disconnect<&TestListener::on_int_value>(&listener);
    observer.notify("test", "key", 20);

    REQUIRE(listener.get_call_count() == 1);
    REQUIRE(listener.get_last_int() == 10);
}

TEST_CASE("ConfigObserver: Disconnect one of multiple handlers", "[config][observer]")
{
    g_callbackLog.clear();
    TestListener listener;
    ConfigObserver observer;

    observer.on_changed("test", "value").connect<&on_string_changed>();
    observer.on_changed("test", "value").connect<&TestListener::on_string_value>(&listener);

    observer.on_changed("test", "value").disconnect<&on_string_changed>();
    observer.notify("test", "value", std::string("hello"));

    REQUIRE(g_callbackLog.empty());
    REQUIRE(listener.get_call_count() == 1);
    REQUIRE(listener.get_last_string() == "hello");
}

TEST_CASE("ConfigObserver: Disconnect one instance leaves others", "[config][observer]")
{
    TestListener listener1;
    TestListener listener2;
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&TestListener::on_int_value>(&listener1);
    observer.on_changed("test", "key").connect<&TestListener::on_int_value>(&listener2);

    observer.on_changed("test", "key").disconnect<&TestListener::on_int_value>(&listener1);
    observer.notify("test", "key", 42);

    REQUIRE(listener1.get_call_count() == 0);
    REQUIRE(listener2.get_call_count() == 1);
    REQUIRE(listener2.get_last_int() == 42);
}

TEST_CASE("ConfigObserver: Disconnect non-existent handler does nothing", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&on_string_changed>();
    observer.on_changed("test", "key").disconnect<&on_float_changed>();
    observer.notify("test", "key", std::string("still works"));

    REQUIRE(g_callbackLog.size() == 1);
    REQUIRE(g_callbackLog[0] == "string:still works");
}

TEST_CASE("ConfigObserver: Disconnect from non-existent key does nothing", "[config][observer]")
{
    ConfigObserver observer;

    // Should not crash
    observer.on_changed("nonexistent", "key").disconnect<&on_string_changed>();
}

TEST_CASE("ConfigObserver: Reconnect after disconnect", "[config][observer]")
{
    g_callbackLog.clear();
    ConfigObserver observer;

    observer.on_changed("test", "key").connect<&on_string_changed>();
    observer.notify("test", "key", std::string("first"));

    observer.on_changed("test", "key").disconnect<&on_string_changed>();
    observer.notify("test", "key", std::string("second"));

    observer.on_changed("test", "key").connect<&on_string_changed>();
    observer.notify("test", "key", std::string("third"));

    REQUIRE(g_callbackLog.size() == 2);
    REQUIRE(g_callbackLog[0] == "string:first");
    REQUIRE(g_callbackLog[1] == "string:third");
}
