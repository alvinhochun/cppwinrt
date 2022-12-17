#include "pch.h"
#include "catch.hpp"
#include "winrt/Windows.Globalization.h"
#include <ctime>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Globalization;
using namespace std::chrono;

namespace winrt
{
    bool operator==(const FILETIME& lhs, const FILETIME& rhs) noexcept
    {
        return lhs.dwHighDateTime == rhs.dwHighDateTime &&
            lhs.dwLowDateTime == rhs.dwLowDateTime;
    }
}

namespace Catch
{
    template<typename Rep, typename Period>
    std::string toString(const duration<Rep, Period>& value_)
    {
        nanoseconds value = value_;
        std::string result;
        if (value >= seconds{ 1 })
        {
            auto count = duration_cast<seconds>(value).count();
            result.append(std::to_string(count));
            result.append(" seconds");
            value %= seconds{ 1 };
        }
        assert(value < seconds{ 1 });
        if (value >= nanoseconds{ 1 })
        {
            auto count = duration_cast<nanoseconds>(value).count();
            if (!result.empty()) { result.append(", "); }
            result.append(std::to_string(count));
            result.append(" nanoseconds");
            value %= nanoseconds{ 1 };
        }
        return result;
    }
}

TEST_CASE("clock, now")
{
    Calendar calendar;
    calendar.SetToNow();
    auto time = clock::now();
    auto diff = calendar.GetDateTime() - time;
    REQUIRE(abs(diff) < milliseconds{ 100 });
}

TEST_CASE("clock, units")
{
    auto time = clock::now();
    Calendar calendar;
    calendar.SetDateTime(time);

    // Add 5 minutes to both to verify that units conversion is working properly
    calendar.AddMinutes(5);
    REQUIRE(calendar.GetDateTime() == (time + 5min));
}

TEST_CASE("clock, time_t")
{
    // Round trip from DateTime to time_t and back.
    // confirm that nothing happens other than truncating the fractional seconds
    const DateTime now_dt = clock::now();
    REQUIRE(clock::from_time_t(clock::to_time_t(now_dt)) == time_point_cast<seconds>(now_dt));

    // Same thing in reverse
    const time_t now_tt = time(nullptr);
    REQUIRE(clock::to_time_t(clock::from_time_t(now_tt)) == now_tt);

    while (clock::now().time_since_epoch().count() % 10000000 < 9990000) {
        Sleep(0);
    }
    for (int i = 0; i < 10; i++) {
    // Conversions are verified to be consistent. Now, verify that we're correctly converting epochs
    const auto now_tt2 = time(nullptr);
    const auto now_dt2 = clock::now();
    const auto now_tt3 = time(nullptr);
    const auto now_dt3 = clock::now();
    Catch::cout() << "now_tt2: " << now_tt2 << std::endl;
    Catch::cout() << "now_dt2: " << now_dt2.time_since_epoch().count() << std::endl;
    Catch::cout() << "now_tt3: " << now_tt3 << std::endl;
    Catch::cout() << "now_dt3: " << now_dt3.time_since_epoch().count() << std::endl;
    Catch::cout() << "clock::from_time_t(now_tt2): " <<  clock::from_time_t(time(nullptr)).time_since_epoch().count() << std::endl;
    const auto diff = duration_cast<milliseconds>(abs(now_dt2 - clock::from_time_t(now_tt2))).count();
    CHECK(diff < 1000);
    }
}

TEST_CASE("clock, FILETIME")
{
    // Round trip conversions
    const DateTime now_dt = clock::now();
    REQUIRE(clock::from_file_time(clock::to_file_time(now_dt)) == now_dt);

    FILETIME now_ft;
    ::GetSystemTimePreciseAsFileTime(&now_ft);
    REQUIRE(clock::to_file_time(clock::from_file_time(now_ft)) == now_ft);

    // Verify epoch
    ::GetSystemTimePreciseAsFileTime(&now_ft);
    const auto diff = abs(clock::now() - clock::from_file_time(now_ft));
    REQUIRE(diff < milliseconds{ 100 });
}

TEST_CASE("clock, system_clock")
{
    DateTime const now_dt = clock::now();
    auto const now_sys = system_clock::now();

    // Round trip DateTime to std::chrono::system_clock::time_point and back
    REQUIRE(clock::from_sys(clock::to_sys(now_dt)) == now_dt);

    // Round trip other direction
    REQUIRE(clock::to_sys(clock::from_sys(now_sys)) == now_sys);

    // Round trip with custom resolution
    {
        auto const now_dt_sec = time_point_cast<seconds>(now_dt);
        REQUIRE(clock::from_sys(clock::to_sys(now_dt_sec)) == now_dt_sec);
    }
    {
        auto const now_dt_mins = time_point_cast<minutes>(now_dt);
        REQUIRE(clock::from_sys(clock::to_sys(now_dt_mins)) == now_dt_mins);
    }
    {
        auto const now_sys_sec = time_point_cast<seconds>(now_sys);
        REQUIRE(clock::to_sys(clock::from_sys(now_sys_sec)) == now_sys_sec);
    }
    {
        auto const now_sys_mins = time_point_cast<seconds>(now_sys);
        REQUIRE(clock::to_sys(clock::from_sys(now_sys_mins)) == now_sys_mins);
    }

    // Verify that the epoch calculations are correct.
    {
        auto const diff = now_dt - clock::from_sys(now_sys);
        REQUIRE(abs(diff) < milliseconds{ 100 });
    }
    {
        auto const diff = now_sys - clock::to_sys(now_dt);
        REQUIRE(abs(diff) < milliseconds{ 100 });
    }
}
