#include <catch.hpp>

#include "utilities/Serializer.hpp"

TEST_CASE("Message with message_id == 0 and empty message", "[Serializer]")
{
    const trade::types::EmptyMessage empty_message;

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::invalid_message_id,
        empty_message
    );

    const auto [message_id, message] = trade::utilities::Serializer::deserialize<trade::types::EmptyMessage>(serialized);

    REQUIRE(message_id == trade::types::MessageID::invalid_message_id);
}

TEST_CASE("Message with message_id != 0 and non-empty message", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::unix_sig,
        unix_sig
    );

    const auto [message_id, message] = trade::utilities::Serializer::deserialize<trade::types::UnixSig>(serialized);

    REQUIRE(message_id == trade::types::MessageID::unix_sig);
    REQUIRE(message.sig() == 2);
}

TEST_CASE("Message with an out-of-range message_id", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        static_cast<trade::types::MessageID>(trade::types::MessageID_MAX + 1),
        unix_sig
    );

    const auto [message_id, message] = trade::utilities::Serializer::deserialize<trade::types::UnixSig>(serialized);

    REQUIRE(message_id == trade::types::MessageID::invalid_message_id);
    REQUIRE(message.sig() == 0); /// The returned message shall be empty.
}