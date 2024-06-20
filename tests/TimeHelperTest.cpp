#include <catch.hpp>

#include "utilities/TimeHelper.hpp"

#ifndef LIB_DATE_SUPPORT
SECTION("TimeHelper support warning")
{
    WARN("TimeHelper is not supported on Windows platform yet");
}
#else
TEST_CASE("Time conversion correctness verification", "[TimeHelper]")
{
    SECTION("ProtobufTime to int64")
    {
        google::protobuf::Timestamp timestamp;

        timestamp.set_seconds(946684800); // 2000-01-01 00:00:00.
        timestamp.set_nanos(0);           // 000 milliseconds.

        CHECK(trade::utilities::ToTime<int64_t>()(timestamp, "Asia/Shanghai") == 20000101080000000);
        CHECK(trade::utilities::ToTime<int64_t>()(timestamp, "Asia/Tokyo") == 20000101090000000);

        timestamp.set_seconds(946753445); // 2000-01-02 03:04:05.
        timestamp.set_nanos(678000000);   // 678 milliseconds.

        CHECK(trade::utilities::ToTime<int64_t>()(timestamp, "Asia/Shanghai") == 20000102030405678);
        CHECK(trade::utilities::ToTime<int64_t>()(timestamp, "Asia/Tokyo") == 20000102040405678);
    }

    SECTION("ProtobufTime to SQLiteDatetime")
    {
        google::protobuf::Timestamp timestamp;

        timestamp.set_seconds(946684800); // 2000-01-01 00:00:00.
        timestamp.set_nanos(0);           // 000 milliseconds.

        CHECK(trade::utilities::ToTime<std::string>()(timestamp, "Asia/Shanghai") == "2000-01-01 08:00:00.000");
        CHECK(trade::utilities::ToTime<std::string>()(timestamp, "Asia/Tokyo") == "2000-01-01 09:00:00.000");

        timestamp.set_seconds(946753445); // 2000-01-02 03:04:05.
        timestamp.set_nanos(678000000);   // 678 milliseconds.

        CHECK(trade::utilities::ToTime<std::string>()(timestamp, "Asia/Shanghai") == "2000-01-02 03:04:05.678");
        CHECK(trade::utilities::ToTime<std::string>()(timestamp, "Asia/Tokyo") == "2000-01-02 04:04:05.678");
    }

    SECTION("SQLiteDatetime to ProtobufTime")
    {
        std::shared_ptr<google::protobuf::Timestamp> timestamp;

        timestamp.reset(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-01 08:00:00.000", "Asia/Shanghai"));

        CHECK(timestamp->seconds() == 946684800); /// 2000-01-01 00:00:00.
        CHECK(timestamp->nanos() == 0);           /// 000 milliseconds.

        timestamp.reset(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-01 09:00:00.000", "Asia/Tokyo"));

        CHECK(timestamp->seconds() == 946684800); /// 2000-01-01 00:00:00.
        CHECK(timestamp->nanos() == 0);           /// 000 milliseconds.

        timestamp.reset(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-02 03:04:05.678", "Asia/Shanghai"));

        CHECK(timestamp->seconds() == 946753445); /// 2000-01-02 03:04:05.
        CHECK(timestamp->nanos() == 678000000);   /// 678 milliseconds.

        timestamp.reset(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-02 04:04:05.678", "Asia/Tokyo"));

        CHECK(timestamp->seconds() == 946753445); /// 2000-01-02 03:04:05.
        CHECK(timestamp->nanos() == 678000000);   /// 678 milliseconds.
    }
}
#endif
