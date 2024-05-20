#pragma once

#include <arpa/inet.h>
#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <zmq.h>

#include "networks.pb.h"

/// TODO: Add debug logging for NetworkHelper classes.

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

/// TODO: Use factory pattern here.
using ZMQContextPtr = std::shared_ptr<void>;

struct ZMQSocketPtrDeleter {
    void operator()(void* ptr) const
    {
        zmq_close(ptr);
    }
};

using ZMQSocketPtr = std::unique_ptr<void, ZMQSocketPtrDeleter>;

/// Encapsulate a ZeroMQ REP (reply) socket for creating a request-reply server.
///
/// This class manages the lifecycle of a ZeroMQ context (if not provided
/// globally) and socket, handling the initialization, binding to an address,
/// and cleanup. It provides methods to receive and send messages using the
/// ZeroMQ request-reply pattern.
class RRServer
{
public:
    explicit RRServer(const std::string& address, const ZMQContextPtr& context = nullptr) : zmq_request()
    {
        if (context != nullptr)
            zmq_context = context;
        else
            zmq_context.reset(zmq_ctx_new(), ZMQContextPtrDeleter());

        zmq_socket.reset(::zmq_socket(zmq_context.get(), ZMQ_REP));

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
        int code;

        do {
            zmq_msg_init(&zmq_request);
            code = zmq_msg_recv(&zmq_request, zmq_socket.get(), 0);
        } while (code < 0);

        return Serializer::deserialize(std::string(static_cast<char*>(zmq_msg_data(&zmq_request)), zmq_msg_size(&zmq_request)));
    }

    void send(const types::MessageID message_id, const google::protobuf::Message& message_body) const
    {
        const auto message = Serializer::serialize(message_id, message_body);
        zmq_send(zmq_socket.get(), message.c_str(), message.size(), ZMQ_DONTWAIT);
    }

private:
    ZMQContextPtr zmq_context;
    ZMQSocketPtr zmq_socket;
    zmq_msg_t zmq_request;
};

/// Encapsulate a ZeroMQ REQ (request) socket for creating a request-reply
/// client.
///
/// This class manages the lifecycle of a ZeroMQ context (if not provided
/// globally) and socket, handling the initialization, connecting to an address,
/// and cleanup. It provides a method to send a request and receive a reply
/// using the ZeroMQ request-reply pattern.
class RRClient
{
public:
    explicit RRClient(const std::string& address, const ZMQContextPtr& context = nullptr) : zmq_response()
    {
        if (context != nullptr)
            zmq_context = context;
        else
            zmq_context.reset(zmq_ctx_new(), ZMQContextPtrDeleter());

        zmq_socket.reset(::zmq_socket(zmq_context.get(), ZMQ_REQ));

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

        /// Return an empty message if the response is invalid.
        if (response_id == types::MessageID::invalid_message_id)
            return ReturnedMessageType {};

        ReturnedMessageType returned_message;
        returned_message.ParseFromString(response_body);

        return returned_message;
    }

private:
    ZMQContextPtr zmq_context;
    ZMQSocketPtr zmq_socket;
    zmq_msg_t zmq_response;
};

template<std::size_t BufferSize = 2048>
class MCServer
{
public:
    explicit MCServer(const std::string& address, const uint16_t port)
        : m_send_addr()
    {
        /// Create what looks like an ordinary UDP socket.
        const auto code = m_sender_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (code < 0) {
            throw std::runtime_error("Failed to create sending socket");
        }

        /// Set up destination address.
        m_send_addr.sin_family      = AF_INET;
        m_send_addr.sin_addr.s_addr = inet_addr(address.c_str());
        m_send_addr.sin_port        = htons(port);
    }
    ~MCServer()
    {
        close(m_sender_fd);
    }

public:
    void send(const std::string& message)
    {
        std::ignore = sendto(m_sender_fd, message.c_str(), message.size(), 0, reinterpret_cast<sockaddr*>(&m_send_addr), sizeof(m_send_addr));
    }

private:
    sockaddr_in m_send_addr;
    int m_sender_fd;
};

template<std::size_t BufferSize = 2048>
class MCClient
{
public:
    explicit MCClient(const std::string& address, const uint16_t port)
        : m_receive_addr(),
          m_mreq(),
          addr_len(sizeof(m_receive_addr))
    {
        /// Create what looks like an ordinary UDP socket.
        auto code = m_receiver_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (code < 0) {
            throw std::runtime_error("Failed to create receiving socket");
        }

        constexpr u_int yes = 1;

        /// Allow multiple sockets to use the same port number.
        code = setsockopt(m_receiver_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (code < 0) {
            throw std::runtime_error(fmt::format("Failed to resue: {}", address));
        }

        /// Set up port.
        m_receive_addr.sin_family      = AF_INET;
        m_receive_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        m_receive_addr.sin_port        = htons(port);

        /// Bind to receive address.
        code = bind(m_receiver_fd, reinterpret_cast<sockaddr*>(&m_receive_addr), sizeof(m_receive_addr));

        if (code < 0) {
            throw std::runtime_error("Failed to bind receiving socket");
        }

        m_mreq.imr_multiaddr.s_addr = inet_addr(address.c_str());
        m_mreq.imr_interface.s_addr = htonl(INADDR_ANY);

        /// Request that the kernel join a multicast group.
        code = setsockopt(m_receiver_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m_mreq, sizeof(m_mreq));
        if (code < 0) {
            throw std::runtime_error("Failed to set receiving socket");
        }

        /// Allocate buffer.
        buffer.reset(new char[BufferSize]);
    }
    ~MCClient()
    {
        close(m_receiver_fd);
    }

public:
    std::string receive()
    {
        const auto received_bytes = recvfrom(m_receiver_fd, buffer.get(), BufferSize, 0, reinterpret_cast<sockaddr*>(&m_receive_addr), &addr_len);

        if (received_bytes < 0) {
            return "";
        }

        return {buffer.get(), static_cast<std::string::size_type>(received_bytes)};
    }

private:
    sockaddr_in m_receive_addr;
    ip_mreq m_mreq;
    int m_receiver_fd;

private:
    std::unique_ptr<char> buffer;
    socklen_t addr_len;
};

} // namespace trade::utilities
