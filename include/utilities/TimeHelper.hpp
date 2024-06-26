#pragma once

/// Disable date support on Windows platforms.
#ifndef WIN32
    #define LIB_DATE_SUPPORT
#endif

#ifdef LIB_DATE_SUPPORT
    #include <date/date.h>
    #include <date/tz.h>
#endif

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>
#include <sstream>
#include <string>

namespace trade::utilities
{

template<typename T>
class ToTime
{
public:
    ToTime() = delete;
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
        date_time << format("%Y-%m-%d %H:%M:%S", local_time);

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
class Now<std::string>
{
public:
    /// Returns the current time in ISO 8601 format.
    std::string operator()() const
    {
        return google::protobuf::util::TimeUtil::ToString(google::protobuf::util::TimeUtil::GetCurrentTime());
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

} // namespace trade::utilities