#include <catch.hpp>

#include "utilities/NetworkHelper.hpp"

TEST_CASE("Message with message_id == 0 and empty message", "[Serializer]")
{
    const trade::types::EmptyMessage empty_message;

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::invalid_message_id,
        empty_message
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    CHECK(message_id == trade::types::MessageID::invalid_message_id);
}

TEST_CASE("Message with message_id != 0 and empty message", "[Serializer]")
{
    const trade::types::EmptyMessage empty_message;

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::unix_sig,
        empty_message
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    CHECK(message_id == trade::types::MessageID::unix_sig);
}

TEST_CASE("Message with message_id == 0 and non-empty message", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::invalid_message_id,
        unix_sig
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    unix_sig.Clear();
    unix_sig.ParseFromString(message_body);

    CHECK(message_id == trade::types::MessageID::invalid_message_id);
    CHECK(unix_sig.sig() == 2);
}

TEST_CASE("Message with message_id != 0 and non-empty message", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        trade::types::MessageID::unix_sig,
        unix_sig
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    unix_sig.Clear();
    unix_sig.ParseFromString(message_body);

    CHECK(message_id == trade::types::MessageID::unix_sig);
    CHECK(unix_sig.sig() == 2);
}

TEST_CASE("Message with an out-of-range message_id", "[Serializer]")
{
    trade::types::UnixSig unix_sig;
    unix_sig.set_sig(2);

    const auto serialized = trade::utilities::Serializer::serialize(
        static_cast<trade::types::MessageID>(trade::types::MessageID_MAX + 1),
        unix_sig
    );

    const auto [message_id, message_body] = trade::utilities::Serializer::deserialize(serialized);

    unix_sig.Clear();
    unix_sig.ParseFromString(message_body);

    CHECK(message_id == trade::types::MessageID::invalid_message_id);
    CHECK(unix_sig.sig() == 2); /// The returned message still contains the original value.
}