#pragma once

#include <google/protobuf/message.h>

#include "networks.pb.h"

namespace trade::utilities
{

class Serializer
{
public:
    /// Serialize a message with a specific Message ID that can be used in
    /// network transferring.
    ///
    /// @note The serialized message follows the following format:
    ///       |<---- 4 bytes ---->|<------ n bytes ------>|
    ///       |    Message ID     |    Raw Message Body   |
    ///
    /// @param message_id The Message ID.
    /// @param message The message.
    /// @return The serialized message.
    static std::string serialize(
        const types::MessageID message_id,
        const google::protobuf::Message& message
    )
    {
        /// Serialize message_id.
        unsigned char message_id_buf[4];
        serialize_int32(message_id_buf, message_id);

        std::string serialized;

        serialized.append(reinterpret_cast<const char*>(message_id_buf), 4);
        serialized.append(message.SerializeAsString());

        return serialized;
    }

    /// Unserialize a message.
    /// @param serialized The serialized message.
    /// @return The message ID and the raw message body. If the serialized
    /// message is invalid, the message ID will be 0 and the message body will
    /// be empty.
    static auto deserialize(const std::string& serialized)
    {
        if (serialized.size() < 4)
            return std::make_tuple(types::MessageID::invalid_message_id, std::string {});

        unsigned char message_id_buf[4];
        memcpy(message_id_buf, serialized.data(), 4);

        const auto message_id = static_cast<types::MessageID>(parse_int32(message_id_buf));

        /// Check if the message is in range.
        if (message_id < types::MessageID_MIN || message_id > types::MessageID_MAX)
            return std::make_tuple(types::MessageID::invalid_message_id, serialized.substr(4, serialized.size() - 4));

        return std::make_tuple(message_id, serialized.substr(4, serialized.size() - 4));
    }

private:
    static void serialize_int32(unsigned char (&buf)[4], const int32_t val)
    {
        const uint32_t uval = val;
        buf[0]              = uval;
        buf[1]              = uval >> 8;
        buf[2]              = uval >> 16;
        buf[3]              = uval >> 24;
    }

    static int32_t parse_int32(const unsigned char (&buf)[4])
    {
        // This prevents buf[i] from being promoted to a signed int.
        const uint32_t u0 = buf[0], u1 = buf[1], u2 = buf[2], u3 = buf[3];
        const uint32_t uval = u0 | u1 << 8 | u2 << 16 | u3 << 24;

        return static_cast<int32_t>(uval);
    }
};

} // namespace trade::utilities
