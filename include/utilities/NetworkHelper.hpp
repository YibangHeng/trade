#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
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
    /// @param message_buffer The serialized message.
    static void serialize(
        const types::MessageID message_id,
        const google::protobuf::Message& message,
        std::vector<u_char>& message_buffer
    )
    {
        /// Serialize message_id.
        u_char message_id_buf[4];
        serialize_int32(message_id_buf, message_id);

        const auto message_body = message.SerializeAsString();

        if (message_body.empty()) {
            message_buffer.clear();
            message_buffer.insert(message_buffer.end(), message_id_buf, message_id_buf + 4);
        }

        message_buffer.clear();
        message_buffer.insert(message_buffer.end(), message_id_buf, message_id_buf + 4);
        message_buffer.insert(message_buffer.end(), message_body.begin(), message_body.end());
    }

    /// Unserialize a message.
    /// @param message_buffer The serialized message.
    /// @return The message ID and the raw message body. If the serialized
    /// message is invalid, the message ID will be 0 and the message body will
    /// be empty.
    static auto deserialize(const std::vector<u_char>& message_buffer)
    {
        if (message_buffer.size() < 4)
            return std::make_tuple(types::MessageID::invalid_message_id, message_buffer.end());

        const auto message_id = static_cast<types::MessageID>(parse_int32(message_buffer.data()));

        /// Check if the message is in range.
        if (message_id < types::MessageID_MIN || message_id > types::MessageID_MAX) [[unlikely]]
            return std::make_tuple(types::MessageID::invalid_message_id, message_buffer.begin() + 4);

        return std::make_tuple(message_id, message_buffer.begin() + HEAD_SIZE<>);
    }

public:
    template<typename T = int>
    constexpr static T HEAD_SIZE = T(4);

private:
    static void serialize_int32(unsigned char (&buf)[4], const int32_t val)
    {
        const uint32_t uval = val;
        buf[0]              = uval;
        buf[1]              = uval >> 8;
        buf[2]              = uval >> 16;
        buf[3]              = uval >> 24;
    }

    static int32_t parse_int32(const u_char buf[4])
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
    explicit RRServer(const std::string& address, const ZMQContextPtr& context = nullptr)
        : m_zmq_request()
    {
        if (context != nullptr)
            m_zmq_context = context;
        else
            m_zmq_context.reset(zmq_ctx_new(), ZMQContextPtrDeleter());

        m_zmq_socket.reset(zmq_socket(m_zmq_context.get(), ZMQ_REP));

        const auto code = zmq_bind(m_zmq_socket.get(), address.c_str());

        if (code != 0) {
            throw std::runtime_error(fmt::format("Failed to bind ZMQ socket at {}: {}", address, std::string(zmq_strerror(errno))));
        }
    }
    ~RRServer()
    {
        zmq_msg_close(&m_zmq_request);
    }

public:
    [[nodiscard]] auto receive(std::vector<u_char>& message_buffer)
    {
        int code;

        do {
            zmq_msg_init(&m_zmq_request);
            code = zmq_msg_recv(&m_zmq_request, m_zmq_socket.get(), 0);
        } while (code < 0);

        /// Copy message to the caller's buffer.
        message_buffer.clear();
        message_buffer.insert(
            message_buffer.begin(),
            static_cast<u_char*>(zmq_msg_data(&m_zmq_request)),
            static_cast<u_char*>(zmq_msg_data(&m_zmq_request)) + zmq_msg_size(&m_zmq_request)
        );

        return Serializer::deserialize(message_buffer);
    }

    void send(const types::MessageID message_id, const google::protobuf::Message& message_body)
    {
        Serializer::serialize(message_id, message_body, m_message_buffer);
        zmq_send(m_zmq_socket.get(), m_message_buffer.data(), m_message_buffer.size(), ZMQ_DONTWAIT);
    }

private:
    ZMQContextPtr m_zmq_context;
    ZMQSocketPtr m_zmq_socket;
    zmq_msg_t m_zmq_request;

private:
    std::vector<u_char> m_message_buffer;
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
    explicit RRClient(const std::string& address, const ZMQContextPtr& context = nullptr) : m_zmq_response()
    {
        if (context != nullptr)
            m_zmq_context = context;
        else
            m_zmq_context.reset(zmq_ctx_new(), ZMQContextPtrDeleter());

        m_zmq_socket.reset(zmq_socket(m_zmq_context.get(), ZMQ_REQ));

        const auto code = zmq_connect(m_zmq_socket.get(), address.c_str());

        if (code != 0) {
            throw std::runtime_error(fmt::format("Failed to connect ZMQ socket to {}: {}", address, std::string(zmq_strerror(errno))));
        }
    }
    ~RRClient()
    {
        zmq_msg_close(&m_zmq_response);
    }

public:
    template<typename ReturnedMessageType>
    ReturnedMessageType request(
        const types::MessageID message_id,
        const google::protobuf::Message& message
    )
    {
        /// Send request.
        Serializer::serialize(message_id, message, m_message_buffer);

        zmq_send(m_zmq_socket.get(), m_message_buffer.data(), m_message_buffer.size(), 0);

        /// Receive response.
        zmq_msg_init(&m_zmq_response);
        zmq_msg_recv(&m_zmq_response, m_zmq_socket.get(), 0);

        /// Copy message to the buffer.
        m_message_buffer.clear();
        m_message_buffer.insert(
            m_message_buffer.begin(),
            static_cast<u_char*>(zmq_msg_data(&m_zmq_response)),
            static_cast<u_char*>(zmq_msg_data(&m_zmq_response)) + zmq_msg_size(&m_zmq_response)
        );

        const auto [response_id, response_body_it] = Serializer::deserialize(m_message_buffer);

        /// Return an empty message if the response is invalid.
        if (response_id == types::MessageID::invalid_message_id)
            return ReturnedMessageType {};

        ReturnedMessageType returned_message;
        returned_message.ParseFromArray(response_body_it.base(), m_message_buffer.size() - Serializer::HEAD_SIZE<>);

        return returned_message;
    }

private:
    ZMQContextPtr m_zmq_context;
    ZMQSocketPtr m_zmq_socket;
    zmq_msg_t m_zmq_response;

private:
    std::vector<u_char> m_message_buffer;
};

/// Encapsulate raw UDP multicast socket.
///
/// This class manages the lifecycle of a raw socket, handling the
/// initialization, and cleanup. It provides methods to send messages using the
/// UDP multicast pattern.
class MCServer
{
public:
    explicit MCServer(const std::string& address, const uint16_t port)
        : m_send_addr()
    {
        /// Create what looks like an ordinary UDP socket.
        const auto code = m_sender_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (code < 0) {
            close(m_sender_fd);
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
    void send(const std::vector<u_char>& message)
    {
        std::ignore = sendto(m_sender_fd, message.data(), message.size(), 0, reinterpret_cast<sockaddr*>(&m_send_addr), sizeof(m_send_addr));
    }

private:
    sockaddr_in m_send_addr;
    int m_sender_fd;
};

/// Encapsulate raw UDP multicast socke.
///
/// This class manages the lifecycle of a raw socket, handling the
/// initialization, and cleanup. It provides methods to receive messages using
/// the UDP multicast pattern.
template<typename T>
class MCClient
{
public:
    explicit MCClient(
        const std::string& address,
        const uint16_t port,
        const std::string& interface_address   = "0.0.0.0",
        [[deprecated]] const bool non_blocking = false
    )
        : m_receive_addr(),
          m_mreq(),
          m_addr_len(sizeof(m_receive_addr))
    {
        /// Create what looks like an ordinary UDP socket.
        auto code = m_receiver_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (code < 0) {
            close(m_receiver_fd);
            throw std::runtime_error(fmt::format("Failed to create receiving socket for {}: {}", address, strerror(errno)));
        }

        constexpr u_int yes = 1;

        /// Allow multiple sockets to use the same port number.
        code = setsockopt(m_receiver_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (code < 0) {
            throw std::runtime_error(fmt::format("Failed to resue address {}: {}", address, strerror(errno)));
        }

        /// Set up destination address.
        m_receive_addr.sin_family      = AF_INET;
        m_receive_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        m_receive_addr.sin_port        = htons(port);

        /// Set up non-blocking IO.
        if (non_blocking) {
            code = fcntl(m_receiver_fd, F_GETFL, 0);

            if (code < 0 || fcntl(m_receiver_fd, F_SETFL, code | O_NONBLOCK) < 0) {
                close(m_receiver_fd);
                throw std::runtime_error(fmt::format("Failed to set receiving socket to non-blocking {}: {}", address, strerror(errno)));
            }
        }

        /// TODO: Make this configurable.
        set_timeout(0, 100000); /// 100ms (= 100,000us)

        /// Bind to receive address.
        code = bind(m_receiver_fd, reinterpret_cast<sockaddr*>(&m_receive_addr), sizeof(m_receive_addr));

        if (code < 0) {
            close(m_receiver_fd);
            throw std::runtime_error(fmt::format("Failed to bind receiving socket at {}:{}: {}", address, port, strerror(errno)));
        }

        /// Set up multicast address.
        m_mreq.imr_multiaddr.s_addr = inet_addr(address.c_str());
        m_mreq.imr_interface.s_addr = inet_addr(interface_address.c_str());

        /// Request that the kernel join a multicast group.
        code = setsockopt(m_receiver_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m_mreq, sizeof(m_mreq));

        if (code < 0) {
            close(m_receiver_fd);
            throw std::runtime_error(fmt::format("Failed to join multicast group {}:{}: {}", address, port, strerror(errno)));
        }
    }
    ~MCClient()
    {
        close(m_receiver_fd);
    }

public:
    ssize_t receive(std::vector<u_char>& message_buffer)
    {
        message_buffer.resize(sizeof(T));
        return recvfrom(m_receiver_fd, message_buffer.data(), sizeof(T), 0, reinterpret_cast<sockaddr*>(&m_receive_addr), &m_addr_len);
    }

private:
    void set_timeout(const time_t seconds, const suseconds_t microseconds) const
    {
        const timeval timeout(seconds, microseconds);

        if (setsockopt(m_receiver_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            close(m_receiver_fd);
            throw std::runtime_error(fmt::format("Failed to set receiving socket timeout: {}", strerror(errno)));
        }
    }

private:
    sockaddr_in m_receive_addr;
    ip_mreq m_mreq;
    int m_receiver_fd;

private:
    socklen_t m_addr_len;
};

} // namespace trade::utilities
