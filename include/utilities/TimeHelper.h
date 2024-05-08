#pragma once

#include <date/date.h>
#include <date/tz.h>
#include <google/protobuf/timestamp.pb.h>
#include <iomanip>
#include <sstream>

namespace trade::utilities
{

class ProtobufTimeToSQLiteDatetime
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
        // Convert Timestamp to Unix epoch time.
        const int64_t seconds = timestamp.seconds();
        const int64_t nanos   = timestamp.nanos();

        // Use date library to handle the timezone.
        const auto zoned_time = make_zoned(date::locate_zone(timezone), std::chrono::system_clock::from_time_t(seconds));
        const auto local_time = zoned_time.get_local_time();
        const auto timeinfo   = date::floor<std::chrono::seconds>(local_time);

        std::stringstream date_time;
        date_time << format("%Y-%m-%d %H:%M:%S", timeinfo);
        date_time << '.' << std::setw(3) << std::setfill('0') << nanos / 1000000;

        return date_time.str();
    }
};

} // namespace trade::utilities