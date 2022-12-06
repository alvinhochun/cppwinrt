#include "pch.h"

using namespace winrt;
using namespace Windows::Foundation;

namespace
{
    //
    // Checks that cancellation propagation works.
    //

    IAsyncAction Action(handle* wait)
    {
        // Do an extra co_await before the resume_on_signal
        // so that there is a race window where we can try to cancel
        // the "co_await resume_on_signal()" before it starts.
        co_await resume_background();

        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();
        SetEvent(wait->get());
        co_await resume_on_signal(GetCurrentProcess()); // never wakes
        REQUIRE(false);
    }

    IAsyncActionWithProgress<int> ActionWithProgress(handle* wait)
    {
        // Do an extra co_await before the resume_on_signal
        // so that there is a race window where we can try to cancel
        // the "co_await resume_on_signal()" before it starts.
        co_await resume_background();

        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();
        SetEvent(wait->get());
        co_await resume_on_signal(GetCurrentProcess()); // never wakes
        REQUIRE(false);
    }

    IAsyncOperation<int> Operation(handle* wait)
    {
        // Do an extra co_await before the resume_on_signal
        // so that there is a race window where we can try to cancel
        // the "co_await resume_on_signal()" before it starts.
        co_await resume_background();

        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();
        SetEvent(wait->get());
        co_await resume_on_signal(GetCurrentProcess()); // never wakes
        REQUIRE(false);
        co_return 1;
    }

    IAsyncOperationWithProgress<int, int> OperationWithProgress(handle* wait)
    {
        // Do an extra co_await before the resume_on_signal
        // so that there is a race window where we can try to cancel
        // the "co_await resume_on_signal()" before it starts.
        co_await resume_background();

        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();
        SetEvent(wait->get());
        co_await resume_on_signal(GetCurrentProcess()); // never wakes
        REQUIRE(false);
        co_return 1;
    }

    // Checking cancellation propagation for resume_after.
    IAsyncAction DelayAction(handle* wait)
    {
        // Do an extra co_await before the resume_on_signal
        // so that there is a race window where we can try to cancel
        // the "co_await resume_after()" before it starts.
        co_await resume_background();

        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();
        SetEvent(wait->get());
        co_await resume_after(std::chrono::hours(1)); // effectively sleep forever
        REQUIRE(false);
    }

    // Checking cancellation propagation for IAsyncAction.
    // We nest "depth" layers deep and then cancel the very
    // deeply-nested IAsyncAction. This validates that propagation
    // carried all the way down, and also lets us verify (via
    // manual debugging) the deep cancellation doesn't cause us to
    // blow up the stack on deeply nested cancellation.
    IAsyncAction ActionAction(handle* wait, int depth)
    {
        // Do an extra co_await before the resume_on_signal
        // so that there is a race window where we can try to cancel
        // the "co_await ActionAction()" before it starts.
        co_await resume_background();

        auto cancel = co_await get_cancellation_token();
        cancel.enable_propagation();
        if (depth > 0)
        {
            co_await ActionAction(wait, depth - 1);
        }
        else
        {
            co_await Action(wait);
        }
        REQUIRE(false);
    }

    template <typename F>
    void Check(F make, bool should_wait)
    {
        handle completed{ CreateEvent(nullptr, true, false, nullptr) };
        handle wait{ CreateEvent(nullptr, true, false, nullptr) };
        auto async = make(&wait);
        REQUIRE(async.Status() == AsyncStatus::Started);

        async.Completed([&](auto&& sender, AsyncStatus status)
            {
                REQUIRE(async == sender);
                REQUIRE(status == AsyncStatus::Canceled);
                SetEvent(completed.get());
            });

        if (should_wait)
        {
            REQUIRE(WaitForSingleObject(wait.get(), IsDebuggerPresent() ? INFINITE : 2000) == WAIT_OBJECT_0);
        }
        async.Cancel();

        // Wait indefinitely if a debugger is present, to make it easier to debug this test.
        REQUIRE(WaitForSingleObject(completed.get(), IsDebuggerPresent() ? INFINITE : 2000) == WAIT_OBJECT_0);

        REQUIRE(async.Status() == AsyncStatus::Canceled);
        REQUIRE(async.ErrorCode() == HRESULT_FROM_WIN32(ERROR_CANCELLED));
        REQUIRE_THROWS_AS(async.GetResults(), hresult_canceled);
    }

    struct wait_t
    {
        static constexpr bool value = true;
    };

    struct no_wait_t
    {
        static constexpr bool value = false;
    };
}

#if defined(__clang__) && defined(_MSC_VER)
// FIXME: Test is known to segfault when built with Clang.
TEMPLATE_TEST_CASE("async_propagate_cancel", "[.clang-crash]", wait_t, no_wait_t)
#else
TEMPLATE_TEST_CASE("async_propagate_cancel", "", wait_t, no_wait_t)
#endif
{
#define CHECK_CASE(a) \
    SECTION("CHECK_CASE(" #a ")") { \
        Check(a, TestType::value); \
    }

    CHECK_CASE(Action);
    CHECK_CASE(ActionWithProgress);
    CHECK_CASE(Operation);
    CHECK_CASE(OperationWithProgress);
    CHECK_CASE(DelayAction);
    CHECK_CASE([](handle* wait) { return ActionAction(wait, 10); });
}
