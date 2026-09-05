#include "input/mouse_capture_controller.hpp"

#include <chrono>
#include <deque>

#include <catch2/catch_test_macros.hpp>

namespace
{
    using namespace std::chrono_literals;
    using vkShade::MouseCaptureAttempt;
    using vkShade::MouseCaptureBackend;
    using vkShade::MouseCaptureController;
    using vkShade::MouseCaptureStatus;
    using vkShade::MouseInputInhibitor;

    class FakeBackend final : public MouseCaptureBackend
    {
    public:
        MouseCaptureAttempt acquire() override
        {
            ++acquireCalls;
            MouseCaptureAttempt result = attempts.empty()
                ? MouseCaptureAttempt {MouseCaptureStatus::Active, false}
                : attempts.front();
            if (!attempts.empty())
                attempts.pop_front();
            currentStatus = result.status;
            return result;
        }

        void release() override
        {
            ++releaseCalls;
            currentStatus = MouseCaptureStatus::Inactive;
        }

        MouseCaptureStatus get_status() const override
        {
            return currentStatus;
        }

        std::deque<MouseCaptureAttempt> attempts;
        MouseCaptureStatus currentStatus {MouseCaptureStatus::Inactive};
        int acquireCalls {0};
        int releaseCalls {0};
    };

    class FakeInhibitor final : public MouseInputInhibitor
    {
    public:
        bool inhibit() override
        {
            ++inhibitCalls;
            return available;
        }

        void restore() override
        {
            ++restoreCalls;
        }

        bool available {true};
        int inhibitCalls {0};
        int restoreCalls {0};
    };
}

TEST_CASE("Mouse capture transitions application and backend once")
{
    FakeBackend backend;
    FakeInhibitor inhibitor;
    MouseCaptureController controller(backend, inhibitor, 100ms);
    const auto start = MouseCaptureController::TimePoint {};

    controller.set_requested(true, start);

    CHECK(controller.is_requested());
    CHECK(controller.is_application_inhibited());
    CHECK(controller.get_status() == MouseCaptureStatus::Active);
    CHECK(inhibitor.inhibitCalls == 1);
    CHECK(backend.acquireCalls == 1);

    controller.set_requested(true, start + 1ms);
    controller.update(start + 2ms);

    CHECK(inhibitor.inhibitCalls == 1);
    CHECK(backend.acquireCalls == 1);

    controller.set_requested(false, start + 3ms);

    CHECK_FALSE(controller.is_requested());
    CHECK_FALSE(controller.is_application_inhibited());
    CHECK(controller.get_status() == MouseCaptureStatus::Inactive);
    CHECK(backend.releaseCalls == 1);
    CHECK(inhibitor.restoreCalls == 1);
}

TEST_CASE("Mouse capture retries only the failed backend acquisition")
{
    FakeBackend backend;
    FakeInhibitor inhibitor;
    backend.attempts = {
        {MouseCaptureStatus::Inactive, true},
        {MouseCaptureStatus::Active, false},
    };
    MouseCaptureController controller(backend, inhibitor, 100ms);
    const auto start = MouseCaptureController::TimePoint {};

    controller.set_requested(true, start);

    CHECK(controller.is_requested());
    CHECK(controller.is_application_inhibited());
    CHECK(controller.get_status() == MouseCaptureStatus::Inactive);
    CHECK(inhibitor.inhibitCalls == 1);
    CHECK(backend.acquireCalls == 1);

    controller.update(start + 99ms);
    CHECK(backend.acquireCalls == 1);

    controller.update(start + 100ms);
    CHECK(controller.get_status() == MouseCaptureStatus::Active);
    CHECK(backend.acquireCalls == 2);
    CHECK(inhibitor.inhibitCalls == 1);
}

TEST_CASE("Mouse capture reports unavailable application inhibition independently")
{
    FakeBackend backend;
    FakeInhibitor inhibitor;
    inhibitor.available = false;
    MouseCaptureController controller(backend, inhibitor, 100ms);
    const auto start = MouseCaptureController::TimePoint {};

    controller.set_requested(true, start);

    CHECK(controller.is_requested());
    CHECK_FALSE(controller.is_application_inhibited());
    CHECK(controller.get_status() == MouseCaptureStatus::Active);
    CHECK(inhibitor.inhibitCalls == 1);
    CHECK(backend.acquireCalls == 1);

    controller.update(start + 100ms);
    controller.set_requested(false, start + 101ms);

    CHECK(inhibitor.inhibitCalls == 1);
    CHECK(inhibitor.restoreCalls == 0);
    CHECK(backend.releaseCalls == 1);
}

TEST_CASE("Mouse capture releases backend state after a failed acquisition")
{
    FakeBackend backend;
    FakeInhibitor inhibitor;
    backend.attempts = {{MouseCaptureStatus::Inactive, true}};
    MouseCaptureController controller(backend, inhibitor, 100ms);
    const auto start = MouseCaptureController::TimePoint {};

    controller.set_requested(true, start);
    controller.set_requested(false, start + 1ms);

    CHECK(backend.acquireCalls == 1);
    CHECK(backend.releaseCalls == 1);
    CHECK(inhibitor.restoreCalls == 1);
}

TEST_CASE("Pending mouse capture waits for asynchronous backend activation")
{
    FakeBackend backend;
    FakeInhibitor inhibitor;
    backend.attempts = {{MouseCaptureStatus::Pending, true}};
    MouseCaptureController controller(backend, inhibitor, 100ms);
    const auto start = MouseCaptureController::TimePoint {};

    controller.set_requested(true, start);
    controller.update(start + 1s);

    CHECK(controller.get_status() == MouseCaptureStatus::Pending);
    CHECK(backend.acquireCalls == 1);

    backend.currentStatus = MouseCaptureStatus::Active;
    controller.update(start + 1001ms);

    CHECK(controller.get_status() == MouseCaptureStatus::Active);
    CHECK(backend.acquireCalls == 1);
}

TEST_CASE("Unavailable mouse capture can recover when backend capability appears")
{
    FakeBackend backend;
    FakeInhibitor inhibitor;
    backend.attempts = {{MouseCaptureStatus::Unavailable, false}};
    MouseCaptureController controller(backend, inhibitor, 100ms);
    const auto start = MouseCaptureController::TimePoint {};

    controller.set_requested(true, start);
    controller.update(start + 1s);

    CHECK(controller.get_status() == MouseCaptureStatus::Unavailable);
    CHECK(backend.acquireCalls == 1);

    backend.currentStatus = MouseCaptureStatus::Inactive;
    controller.update(start + 1001ms);

    CHECK(controller.get_status() == MouseCaptureStatus::Active);
    CHECK(backend.acquireCalls == 2);
    CHECK(inhibitor.inhibitCalls == 1);
}
