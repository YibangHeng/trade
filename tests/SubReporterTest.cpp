#include <catch.hpp>
#include <muduo/base/Logging.h>
#include <muduo/net/TcpClient.h>
#include <thread>

#include "AppBase.hpp"
#include "libreporter/SubReporter.h"

constexpr size_t num_threads = 1;

class SubReporterClient final: private trade::AppBase<>
{
public:
    explicit SubReporterClient(const muduo::net::InetAddress& server_addr)
        : AppBase("SubReporterClient"),
          m_codec([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& receive_time) { m_dispatcher.on_protobuf_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(receive_time)); }),
          m_dispatcher([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& timestamp) { on_invalied_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(timestamp)); })
    {
        muduo::Logger::setLogLevel(muduo::Logger::NUM_LOG_LEVELS);

        m_dispatcher.register_message_callback<trade::types::NewSubscribeRsp>([this](const muduo::net::TcpConnectionPtr& conn, const trade::utilities::MessagePtr& message, muduo::Timestamp timestamp) { on_new_subscribe_rsp(conn, message, timestamp); });

        /// Start event loop.
        m_event_loop_future = std::async(std::launch::async, [this, server_addr] {
            m_loop   = std::make_shared<muduo::net::EventLoop>();
            m_client = std::make_shared<muduo::net::TcpClient>(m_loop.get(), server_addr, "SubReporterClient");

            m_client->setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn) { on_connected(conn); });
            m_client->setMessageCallback([this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, const muduo::Timestamp receive_time) { m_codec.on_message(conn, buf, receive_time); });

            m_client->connect();

            /// Enter event loop until quit.
            m_loop->loop();

            m_client.reset();
            m_loop.reset();
        });

        m_mutex.lock();
    }

    ~SubReporterClient() override
    {
        m_loop->quit();
        m_event_loop_future.wait();
    }

public:
    /// Wait until the response is received.
    void wait()
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this] { return true; });
    }

private:
    void on_connected(const muduo::net::TcpConnectionPtr& conn)
    {
        if (conn->connected()) {
            trade::types::NewSubscribeReq req;

            req.set_request_id(ticker_taper());
            req.set_app_name(app_name());
            req.add_symbols_subscribe("600875");
            req.add_symbol_unsubscribe("000001");

            trade::utilities::ProtobufCodec::send(conn, req);
        }
        else {
            CHECK(false);
        }
    }

    void on_new_subscribe_rsp(
        const muduo::net::TcpConnectionPtr& conn,
        const trade::utilities::MessagePtr& message,
        muduo::Timestamp
    )
    {
        CHECK(message->GetDescriptor() == trade::types::NewSubscribeRsp::descriptor());
        const auto new_subscribe_rsp = std::dynamic_pointer_cast<trade::types::NewSubscribeRsp>(message);

        CHECK(new_subscribe_rsp->subscribed_symbol_size() == 1);
        CHECK(new_subscribe_rsp->subscribed_symbol(0) == "600875");

        m_mutex.unlock();
        m_cv.notify_one();
    }

    void on_invalied_message(const muduo::net::TcpConnectionPtr& conn, const trade::utilities::MessagePtr&, muduo::Timestamp) const
    {
        logger->warn("Invalid message received from {}", conn->peerAddress().toIpPort());
    }

private:
    trade::utilities::ProtobufCodec m_codec;
    trade::utilities::ProtobufDispatcher m_dispatcher;
    std::shared_ptr<muduo::net::EventLoop> m_loop;
    std::shared_ptr<muduo::net::TcpClient> m_client;
    std::future<void> m_event_loop_future;

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

TEST_CASE("Communication between SubReporterServer and SubReporterClient", "[SubReporter]")
{
    SECTION("Sending and receiving messages with M:1 connections")
    {
        /// Server side.
        trade::reporter::SubReporter sub_reporter(10000);

        /// Client side.
        auto client_worker = [] {
            SubReporterClient client(muduo::net::InetAddress("127.0.0.1", 10000));
            client.wait();
        };

        std::array<std::thread, num_threads> client_threads;

        for (auto& client_thread : client_threads) {
            client_thread = std::thread(client_worker);
        }

        for (auto& client_thread : client_threads)
            client_thread.join();
    }
}
