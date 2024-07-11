#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/message_lite.h>
#include <map>
#include <third/muduo/muduo/base/Timestamp.h>
#include <third/muduo/muduo/base/noncopyable.h>
#include <third/muduo/muduo/net/Buffer.h>
#include <third/muduo/muduo/net/Callbacks.h>
#include <utility>
#include <zlib.h>

namespace trade::utilities
{

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class ICallback: muduo::noncopyable
{
public:
    virtual ~ICallback() = default;
    virtual void on_message(
        const muduo::net::TcpConnectionPtr&,
        const MessagePtr& message,
        muduo::Timestamp
    ) const
        = 0;
};

template<typename T>
class CallbackT final: public ICallback
{
    static_assert(std::is_base_of_v<google::protobuf::Message, T>, "T must be derived from google::protobuf::Message");

public:
    using ProtobufMessageTCallback = std::function<void(const muduo::net::TcpConnectionPtr&, const std::shared_ptr<T>&, muduo::Timestamp)>;

    explicit CallbackT(const ProtobufMessageTCallback& callback)
        : m_callback(callback)
    {
    }

public:
    void on_message(
        const muduo::net::TcpConnectionPtr& conn,
        const MessagePtr& message,
        muduo::Timestamp receive_time
    ) const override
    {
        std::shared_ptr<T> concrete = muduo::down_pointer_cast<T>(message);
        assert(concrete != NULL);
        m_callback(conn, concrete, receive_time);
    }

private:
    ProtobufMessageTCallback m_callback;
};

class ProtobufCodec: muduo::noncopyable
{
public:
    enum ErrorCode
    {
        kNoError = 0,
        kInvalidLength,
        kCheckSumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
    };

    /// Called when a new message is parsed.
    using ProtobufMessageCallback = std::function<void(
        const muduo::net::TcpConnectionPtr&,
        const MessagePtr&,
        muduo::Timestamp
    )>;

    /// Called when parse a message failed.
    using ErrorCallback = std::function<void(
        const muduo::net::TcpConnectionPtr&,
        muduo::net::Buffer*,
        muduo::Timestamp,
        ErrorCode
    )>;

    explicit ProtobufCodec(ProtobufMessageCallback messageCb)
        : m_message_callback(std::move(messageCb)),
          m_error_callback(default_error_callback)
    {
    }

    ProtobufCodec(ProtobufMessageCallback messageCb, ErrorCallback errorCb)
        : m_message_callback(std::move(messageCb)),
          m_error_callback(std::move(errorCb))
    {
    }

    void on_message(
        const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf,
        const muduo::Timestamp receive_time
    ) const
    {
        while (buf->readableBytes() >= m_min_message_len + m_header_len) {
            const int32_t len = buf->peekInt32();
            if (len > m_max_message_len || len < m_min_message_len) {
                m_error_callback(conn, buf, receive_time, kInvalidLength);
                break;
            }

            if (buf->readableBytes() >= google::protobuf::implicit_cast<size_t>(len + m_header_len)) {
                ErrorCode errorCode = kNoError;
                MessagePtr message  = parse(buf->peek() + m_header_len, len, &errorCode);
                if (errorCode == kNoError && message) {
                    m_message_callback(conn, message, receive_time);
                    buf->retrieve(m_header_len + len);
                }
                else {
                    m_error_callback(conn, buf, receive_time, errorCode);
                    break;
                }
            }
            else {
                break;
            }
        }
    }

    static void send(
        const muduo::net::TcpConnectionPtr& conn,
        const google::protobuf::Message& message
    )
    {
        muduo::net::Buffer buf;
        fill_empty_buffer(&buf, message);
        conn->send(&buf);
    }

private:
    static int32_t as_int32(const char* buf)
    {
        int32_t be32 = 0;
        memcpy(&be32, buf, sizeof(be32));
        return static_cast<int32_t>(muduo::net::sockets::networkToHost32(be32));
    }

    static std::string error_code_to_string(const ErrorCode error_code)
    {
        switch (error_code) {
        case kNoError:
            return "kNoErrorStr";
        case kInvalidLength:
            return "kInvalidLengthStr";
        case kCheckSumError:
            return "kCheckSumErrorStr";
        case kInvalidNameLen:
            return "kInvalidNameLenStr";
        case kUnknownMessageType:
            return "kUnknownMessageTypeStr";
        case kParseError:
            return "kParseErrorStr";
        default:
            return "kUnknownErrorStr";
        }
    }

    static void fill_empty_buffer(muduo::net::Buffer* buf, const google::protobuf::Message& message)
    {
        assert(buf->readableBytes() == 0);

        const std::string& typeName = message.GetTypeName();
        const auto nameLen          = static_cast<int32_t>(typeName.size() + 1);
        buf->appendInt32(nameLen);
        buf->append(typeName.c_str(), nameLen);

#if GOOGLE_PROTOBUF_VERSION > 3009002
        int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
        int byte_size = message.ByteSize();
#endif
        buf->ensureWritableBytes(byte_size);

        auto* start        = reinterpret_cast<uint8_t*>(buf->beginWrite());
        const uint8_t* end = message.SerializeWithCachedSizesToArray(start);
        if (end - start != byte_size) {
        }
        buf->hasWritten(byte_size);

        const auto checkSum = static_cast<int32_t>(
            ::adler32(1, reinterpret_cast<const Bytef*>(buf->peek()), static_cast<int>(buf->readableBytes()))
        );
        buf->appendInt32(checkSum);
        assert(buf->readableBytes() == sizeof nameLen + nameLen + byte_size + sizeof checkSum);
        const uint32_t len = muduo::net::sockets::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
        buf->prepend(&len, sizeof len);
    }

    static google::protobuf::Message* create_message(const std::string& type_name)
    {
        google::protobuf::Message* message             = nullptr;
        const google::protobuf::Descriptor* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
        if (descriptor) {
            const google::protobuf::Message* prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
            if (prototype) {
                message = prototype->New();
            }
        }
        return message;
    }

    static MessagePtr parse(const char* buf, const int len, ErrorCode* error)
    {
        MessagePtr message;

        // check sum
        const int32_t expectedCheckSum = as_int32(buf + len - m_header_len);
        const auto checkSum            = static_cast<int32_t>(adler32(1, reinterpret_cast<const Bytef*>(buf), len - m_header_len));
        if (checkSum == expectedCheckSum) {
            // get message type name
            const int32_t nameLen = as_int32(buf);
            if (nameLen >= 2 && nameLen <= len - 2 * m_header_len) {
                const std::string typeName(buf + m_header_len, buf + m_header_len + nameLen - 1);
                // create message object
                message.reset(create_message(typeName));
                if (message) {
                    // parse from buffer
                    const char* data      = buf + m_header_len + nameLen;
                    const int32_t dataLen = len - nameLen - 2 * m_header_len;
                    if (message->ParseFromArray(data, dataLen)) {
                        *error = kNoError;
                    }
                    else {
                        *error = kParseError;
                    }
                }
                else {
                    *error = kUnknownMessageType;
                }
            }
            else {
                *error = kInvalidNameLen;
            }
        }
        else {
            *error = kCheckSumError;
        }

        return message;
    }

private:
    static void default_error_callback(
        const muduo::net::TcpConnectionPtr&,
        muduo::net::Buffer*,
        muduo::Timestamp,
        ErrorCode
    )
    {
    }

    ProtobufMessageCallback m_message_callback;
    ErrorCallback m_error_callback;

    static constexpr int m_header_len      = sizeof(int32_t);
    static constexpr int m_min_message_len = 2 * m_header_len + 2;
    static constexpr int m_max_message_len = 64 * 1024 * 1024;
};

class ProtobufDispatcher
{
public:
    using ProtobufMessageCallback = std::function<void(
        const muduo::net::TcpConnectionPtr&,
        const MessagePtr& message,
        muduo::Timestamp
    )>;

    explicit ProtobufDispatcher(ProtobufMessageCallback defaultCb)
        : m_default_callback(std::move(defaultCb))
    {
    }

    void on_protobuf_message(
        const muduo::net::TcpConnectionPtr& conn,
        const MessagePtr& message,
        const muduo::Timestamp receive_time
    ) const
    {
        const auto it = m_callbacks.find(message->GetDescriptor());
        if (it != m_callbacks.end()) {
            it->second->on_message(conn, message, receive_time);
        }
        else {
            m_default_callback(conn, message, receive_time);
        }
    }

    template<typename T>
    void register_message_callback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
    {
        std::shared_ptr<CallbackT<T>> pd(new CallbackT<T>(callback));
        m_callbacks[T::descriptor()] = pd;
    }

private:
    using CallbackMap = std::map<const google::protobuf::Descriptor*, std::shared_ptr<ICallback>>;

    CallbackMap m_callbacks;
    ProtobufMessageCallback m_default_callback;
};

} // namespace trade::utilities
