#include "input/mouse_input_inhibitor_group.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace
{
    using vkShade::MouseInputInhibitor;
    using vkShade::MouseInputInhibitorGroup;

    class RecordingInhibitor final : public MouseInputInhibitor
    {
    public:
        RecordingInhibitor(
            std::string name,
            bool available,
            std::vector<std::string>& calls)
            : m_name(std::move(name))
            , m_available(available)
            , m_calls(calls)
        {
        }

        bool inhibit() override
        {
            m_calls.push_back("inhibit " + m_name);
            return m_available;
        }

        void restore() override
        {
            m_calls.push_back("restore " + m_name);
        }

        void reconcile() override
        {
            m_calls.push_back("reconcile " + m_name);
        }

    private:
        std::string m_name;
        bool m_available;
        std::vector<std::string>& m_calls;
    };
}

TEST_CASE("Mouse input inhibitor group restores only active members in reverse order")
{
    std::vector<std::string> calls;
    MouseInputInhibitorGroup group;
    group.add(std::make_unique<RecordingInhibitor>("SDL", true, calls));
    group.add(std::make_unique<RecordingInhibitor>("Unavailable", false, calls));
    group.add(std::make_unique<RecordingInhibitor>("Wine", true, calls));

    CHECK(group.inhibit());
    group.restore();

    CHECK(calls == std::vector<std::string> {
        "inhibit SDL",
        "inhibit Unavailable",
        "inhibit Wine",
        "restore Wine",
        "restore SDL",
    });
}

TEST_CASE("Mouse input inhibitor group reports when no member is available")
{
    std::vector<std::string> calls;
    MouseInputInhibitorGroup group;
    group.add(std::make_unique<RecordingInhibitor>("Unavailable", false, calls));

    CHECK_FALSE(group.inhibit());
    group.restore();

    CHECK(calls == std::vector<std::string> {"inhibit Unavailable"});
}
