#include <catch.hpp>

#include "utilities/TimeHelper.hpp"

TEST_CASE("ProtobufTime to SQLiteDatetime", "[TimeHelper]")
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

TEST_CASE("SQLiteDatetime to ProtobufTime", "[TimeHelper]")
{
    auto timestamp = std::shared_ptr<google::protobuf::Timestamp>(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-01 08:00:00.000", "Asia/Shanghai"));

    CHECK(timestamp->seconds() == 946684800); /// 2000-01-01 00:00:00.
    CHECK(timestamp->nanos() == 0);           /// 000 milliseconds.

    timestamp = std::shared_ptr<google::protobuf::Timestamp>(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-01 09:00:00.000", "Asia/Tokyo"));

    CHECK(timestamp->seconds() == 946684800); /// 2000-01-01 00:00:00.
    CHECK(timestamp->nanos() == 0);           /// 000 milliseconds.

    timestamp = std::shared_ptr<google::protobuf::Timestamp>(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-02 03:04:05.678", "Asia/Shanghai"));

    CHECK(timestamp->seconds() == 946753445); /// 2000-01-02 03:04:05.
    CHECK(timestamp->nanos() == 678000000);   /// 678 milliseconds.

    timestamp = std::shared_ptr<google::protobuf::Timestamp>(trade::utilities::ToTime<google::protobuf::Timestamp*>()("2000-01-02 04:04:05.678", "Asia/Tokyo"));

    CHECK(timestamp->seconds() == 946753445); /// 2000-01-02 03:04:05.
    CHECK(timestamp->nanos() == 678000000);   /// 678 milliseconds.
}
