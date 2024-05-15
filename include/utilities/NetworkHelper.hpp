#pragma once

#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <zmq.h>

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

struct ZMQContextPtrDeleter {
    void operator()(void* ptr) const
    {
        zmq_ctx_destroy(ptr);
    }
};

/// TODO: Use std::shared_ptr and factory pattern here.
using ZMQContextPtr = std::unique_ptr<void, ZMQContextPtrDeleter>;

struct ZMQSocketPtrDeleter {
    void operator()(void* ptr) const
    {
        zmq_close(ptr);
    }
};

using ZMQSocketPtr = std::unique_ptr<void, ZMQSocketPtrDeleter>;

class RRServer
{
public:
    explicit RRServer(const std::string& address, void* context = nullptr) : zmq_request()
    {
        zmq_context = context != nullptr ? context : zmq_ctx_new();
        zmq_socket.reset(::zmq_socket(zmq_context, ZMQ_REP));

        const auto code = zmq_bind(zmq_socket.get(), address.c_str());

        if (code != 0) {
            throw std::runtime_error(fmt::format("Failed to bind ZMQ socket at {}: {}", address, std::string(zmq_strerror(errno))));
        }
    }
    ~RRServer()
    {
        zmq_msg_close(&zmq_request);
    }

public:
    [[nodiscard]] auto receive()
    {
        zmq_msg_init(&zmq_request);

        const auto code = zmq_msg_recv(&zmq_request, zmq_socket.get(), 0);

        if (code < 0)
            return std::make_tuple(types::MessageID::invalid_message_id, std::string {});

        return Serializer::deserialize(std::string(static_cast<char*>(zmq_msg_data(&zmq_request)), zmq_msg_size(&zmq_request)));
    }

    void send(const types::MessageID message_id, const google::protobuf::Message& message_body) const
    {
        const auto message = Serializer::serialize(message_id, message_body);

        zmq_send(zmq_socket.get(), message.c_str(), message.size(), ZMQ_DONTWAIT);
    }

private:
    void* zmq_context;
    ZMQSocketPtr zmq_socket;
    zmq_msg_t zmq_request;
};

class RRClient
{
public:
    explicit RRClient(const std::string& address, void* context = nullptr) : zmq_response()
    {
        zmq_context = context != nullptr ? context : zmq_ctx_new();
        zmq_socket.reset(::zmq_socket(zmq_context, ZMQ_REQ));

        const auto code = zmq_connect(zmq_socket.get(), address.c_str());

        if (code != 0) {
            throw std::runtime_error(fmt::format("Failed to connect ZMQ socket to {}: {}", address, std::string(zmq_strerror(errno))));
        }
    }
    ~RRClient()
    {
        zmq_msg_close(&zmq_response);
    }

public:
    template<typename ReturnedMessageType>
    ReturnedMessageType request(
        const types::MessageID message_id,
        const google::protobuf::Message& message
    )
    {
        /// Send request.
        const auto message_body = Serializer::serialize(message_id, message);

        zmq_send(zmq_socket.get(), message_body.c_str(), message_body.size(), 0);

        /// Receive response.
        zmq_msg_init(&zmq_response);

        zmq_msg_recv(&zmq_response, zmq_socket.get(), 0);

        const auto [response_id, response_body] = Serializer::deserialize({static_cast<char*>(zmq_msg_data(&zmq_response)), zmq_msg_size(&zmq_response)});

        zmq_msg_close(&zmq_response);

        /// Return an empty message if the response is invalid.
        if (response_id == types::MessageID::invalid_message_id)
            return ReturnedMessageType {};

        ReturnedMessageType returned_message;
        returned_message.ParseFromString(response_body);

        return returned_message;
    }

private:
    void* zmq_context;
    ZMQSocketPtr zmq_socket;
    zmq_msg_t zmq_response;
};

} // namespace trade::utilities
