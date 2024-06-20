#pragma once

/// Disable date support on Windows platforms.
#ifndef WIN32
    #define LIB_DATE_SUPPORT
#endif

#ifdef LIB_DATE_SUPPORT
    #include <date/date.h>
    #include <date/tz.h>
#endif

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>
#include <sstream>
#include <string>

namespace trade::utilities
{

template<typename>
class ToTime
{
public:
    ToTime() = delete;
};

template<>
class ToTime<int64_t>
{
public:
    int64_t operator()(
        const google::protobuf::Timestamp& timestamp,
        const std::string& timezone = "Asia/Shanghai"
    ) const
    {
#ifdef LIB_DATE_SUPPORT
        const auto time_point = std::chrono::system_clock::from_time_t(timestamp.seconds());
        const auto zoned_time = make_zoned(
            date::locate_zone(timezone),
            time_point + std::chrono::nanoseconds(timestamp.nanos())
        );
        const auto local_time  = zoned_time.get_local_time();

        const auto date_point  = date::floor<date::days>(local_time);
        const auto ymd         = date::year_month_day {date_point};
        const auto time        = date::make_time(local_time - date::floor<date::days>(local_time));

        int64_t formatted_time = static_cast<int>(ymd.year());
        formatted_time *= 100;
        formatted_time += static_cast<uint>(ymd.month());
        formatted_time *= 100;
        formatted_time += static_cast<uint>(ymd.day());
        formatted_time *= 100;
        formatted_time += time.hours().count();
        formatted_time *= 100;
        formatted_time += time.minutes().count();
        formatted_time *= 100;
        formatted_time += time.seconds().count();
        formatted_time *= 1000;
        formatted_time += std::chrono::duration_cast<std::chrono::milliseconds>(local_time.time_since_epoch()).count()
                        % 1000;

        return formatted_time;
#else
        return 20000101080000000;
#endif
    }
};

template<>
class ToTime<google::protobuf::Timestamp*>
{
public:
    /// Convert SQLite datetime string to google::protobuf::Timestamp with timezone support.
    /// @param datetime The datetime string in format %Y-%m-%d %H:%M:%S.
    /// @param timezone The timezone to use for conversion.
    /// @return The converted google::protobuf::Timestamp.
    google::protobuf::Timestamp* operator()(
        const std::string& datetime,
        const std::string& timezone = "Asia/Shanghai"
    ) const
    {
#ifdef LIB_DATE_SUPPORT
        date::local_time<std::chrono::nanoseconds> ls;
        std::istringstream {datetime} >> parse("%Y-%m-%d %H:%M:%S", ls);

        /// Attach timezone to the local time.
        const auto utc_tp      = date::locate_zone(timezone)->to_sys(ls);

        auto* timestamp        = new google::protobuf::Timestamp();
        const auto seconds     = std::chrono::duration_cast<std::chrono::seconds>(utc_tp.time_since_epoch()).count();
        const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(utc_tp.time_since_epoch()).count()
                               % std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count();

        timestamp->set_seconds(seconds);
        timestamp->set_nanos(static_cast<int>(nanoseconds));

        return timestamp;
#else
        return new google::protobuf::Timestamp();
#endif
    }
};

template<>
class ToTime<std::string>
{
public:
    /// Convert google::protobuf::Timestamp to SQLite datetime string with timezone support.
    /// @param timestamp The timestamp to convert.
    /// @param timezone The timezone to use for conversion.
    /// @return The converted string in SQLite datetime format.
    std::string operator()(
        const google::protobuf::Timestamp& timestamp,
        const std::string& timezone = "Asia/Shanghai"
    ) const
    {
#ifdef LIB_DATE_SUPPORT
        const auto zoned_time = make_zoned(
            date::locate_zone(timezone),
            std::chrono::system_clock::from_time_t(timestamp.seconds())
        );

        auto local_time = zoned_time.get_local_time();
        local_time += std::chrono::nanoseconds(timestamp.nanos());

        /// In format 2000-01-01 08:00:00.000000000.
        std::stringstream date_time;
        date_time << date::format("%Y-%m-%d %H:%M:%S", local_time);

        /// Remove nanoseconds part.
        return date_time.str().substr(0, date_time.str().size() - 6);
#else
        return "2000-01-01 08:00:00.000";
#endif
    }
};

template<typename>
class Now
{
public:
    Now() = delete;
};

/// See https://protobuf.dev/support/migration/#getcurrenttime.
#undef GetCurrentTime

template<>
class Now<int64_t>
{
public:
    /// Returns the current local time in format 20000101080000000.
    int64_t operator()() const
    {
        const auto now          = std::chrono::system_clock::now();
        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        const auto local_time   = *std::localtime(&now_c);

        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                                % 1000;

        int64_t formatted_time = local_time.tm_year + 1900;
        formatted_time *= 100;
        formatted_time += local_time.tm_mon + 1;
        formatted_time *= 100;
        formatted_time += local_time.tm_mday;
        formatted_time *= 100;
        formatted_time += local_time.tm_hour;
        formatted_time *= 100;
        formatted_time += local_time.tm_min;
        formatted_time *= 100;
        formatted_time += local_time.tm_sec;
        formatted_time *= 1000;
        formatted_time += milliseconds.count();

        return formatted_time;
    }
};

template<>
class Now<std::string>
{
public:
    /// Returns the current local time in format 2000-01-01 08:00:00.000.
    std::string operator()() const
    {
        const auto now          = std::chrono::system_clock::now();

        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        const auto local_time   = *std::localtime(&now_c);

        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                                % 1000;

        return fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03d}", local_time, milliseconds.count());
    }
};

template<>
class Now<google::protobuf::Timestamp*>
{
public:
    /// Returns the current time with nanosecond precision.
    ///
    /// Note: Set the returned value to a protobuf message will hand over the
    /// ownership to the message, which takes the responsibility of freeing it.
    google::protobuf::Timestamp* operator()() const
    {
        return new google::protobuf::Timestamp(google::protobuf::util::TimeUtil::GetCurrentTime());
    }
};

template<typename>
class Date
{
public:
    Date() = delete;
};

template<>
class Date<int64_t>
{
public:
    /// Returns the current local date in format 20000101.
    int64_t operator()() const
    {
        const auto now          = std::chrono::system_clock::now();

        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        const auto local_time   = *std::localtime(&now_c);

        int64_t formatted_time  = local_time.tm_year + 1900;
        formatted_time *= 100;
        formatted_time += local_time.tm_mon + 1;
        formatted_time *= 100;
        formatted_time += local_time.tm_mday;

        return formatted_time;
    }
};

template<>
class Date<std::string>
{
public:
    /// Returns the current local date in format 20000101.
    std::string operator()() const
    {
        const auto now          = std::chrono::system_clock::now();

        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        auto local_time         = *std::localtime(&now_c);

        return fmt::format("{:%Y%m%d}", local_time);
    }
};

} // namespace trade::utilities