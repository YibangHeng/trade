#pragma once

#include <future>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>

#include "networks.pb.h"
#include "orms.pb.h"
#include "utilities/LoginSyncer.hpp"
#include "utilities/ProtobufDispatcher.hpp"
#include "visibility.h"

namespace trade
{

using L2SnapCallBackType          = std::function<void(
    const muduo::net::TcpConnectionPtr&,
    const types::L2Tick&,
    muduo::Timestamp
)>;
using NewSubscribeRspCallBackType = std::function<void(
    const muduo::net::TcpConnectionPtr&,
    const types::NewSubscribeRsp&,
    muduo::Timestamp
)>;
using ConnectCallBackType         = std::function<void(const muduo::net::TcpConnectionPtr&)>;
using DisconnectCallBackType      = std::function<void(const muduo::net::TcpConnectionPtr&)>;

class PUBLIC_API SubReporterClientImpl final: public utilities::LoginSyncer
{
public:
    explicit SubReporterClientImpl(
        const std::string& host                                  = "127.0.0.1",
        const uint16_t port                                      = 10100,
        L2SnapCallBackType&& l2_snap_callback                    = [](const muduo::net::TcpConnectionPtr&, const types::L2Tick&, muduo::Timestamp) {},
        NewSubscribeRspCallBackType&& new_subscribe_rsp_callback = [](const muduo::net::TcpConnectionPtr&, const types::NewSubscribeRsp&, muduo::Timestamp) {},
        ConnectCallBackType&& connect_callback                   = [](const muduo::net::TcpConnectionPtr&) {},
        DisconnectCallBackType&& disconnect_callback             = [](const muduo::net::TcpConnectionPtr&) {}
    ) : l2_snap_callback(std::move(l2_snap_callback)),
        new_subscribe_rsp_callback(std::move(new_subscribe_rsp_callback)),
        connect_callback(std::move(connect_callback)),
        disconnect_callback(std::move(disconnect_callback)),
        m_codec([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& receive_time) { m_dispatcher.on_protobuf_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(receive_time)); }),
        m_dispatcher([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& timestamp) { on_invalied_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(timestamp)); }),
        app_name("SubReporterClientImpl") /// TODO: Make it configurable.
    {
        muduo::net::InetAddress address(host, port);

        muduo::Logger::setLogLevel(muduo::Logger::NUM_LOG_LEVELS);

        m_dispatcher.register_message_callback<types::L2Tick>([this](const muduo::net::TcpConnectionPtr& conn, const utilities::MessagePtr& message, const muduo::Timestamp timestamp) { on_new_l2_snap_arrived(conn, message, timestamp); });
        m_dispatcher.register_message_callback<types::NewSubscribeRsp>([this](const muduo::net::TcpConnectionPtr& conn, const utilities::MessagePtr& message, const muduo::Timestamp timestamp) { on_new_subscribe_rsp(conn, message, timestamp); });

        /// Start event loop.
        m_event_loop_future = std::async(std::launch::async, [this, address] {
            m_loop   = std::make_shared<muduo::net::EventLoop>();
            m_client = std::make_shared<muduo::net::TcpClient>(m_loop.get(), address, "SubReporterClient");

            m_client->setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn) { on_connected(conn); });
            m_client->setMessageCallback([this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, const muduo::Timestamp receive_time) { m_codec.on_message(conn, buf, receive_time); });

            m_client->connect();

            /// Enter event loop until quit.
            m_loop->loop();

            m_client.reset();
            m_loop.reset();
        });

        LoginSyncer::start_login();
    }
    ~SubReporterClientImpl() override
    {
        m_is_running = false;

        m_loop->quit();
        m_event_loop_future.wait();
    }

public:
    template<std::ranges::range SymbolListType>
    void subscribe(
        const SymbolListType& subscribes,
        const SymbolListType& unsubscribes = {},
        const bool request_last_data       = false
    )
    {
        types::NewSubscribeReq new_subscribe_req;

        new_subscribe_req.set_request_id(ticker_taper());
        new_subscribe_req.set_app_name(app_name);
        std::ranges::for_each(subscribes, [&new_subscribe_req](const auto& symbol) { new_subscribe_req.add_symbols_to_subscribe(symbol); });
        std::ranges::for_each(unsubscribes, [&new_subscribe_req](const auto& symbol) { new_subscribe_req.add_symbols_to_unsubscribe(symbol); });
        new_subscribe_req.set_request_last_data(request_last_data);

        utilities::ProtobufCodec::send(m_conn, new_subscribe_req);
    }

private:
    void on_connected(const muduo::net::TcpConnectionPtr& conn)
    {
        if (conn->connected()) {
            m_is_running = true;

            /// Set connection.
            m_conn = conn;
            connect_callback(conn);

            notify_login_success();

            subscribe(m_subscribed_symbols);
        }
        else {
            m_conn.reset();
            disconnect_callback(conn);

            if (m_is_running) /// Try to reconnect.
                m_client->connect();
            else
                notify_login_failure("connection failed");
        }
    }

    void on_new_l2_snap_arrived(
        const muduo::net::TcpConnectionPtr& conn,
        const utilities::MessagePtr& message,
        const muduo::Timestamp timestamp
    ) const
    {
        const auto& l2_tick = std::dynamic_pointer_cast<types::L2Tick>(message);
        if (l2_tick != nullptr)
            l2_snap_callback(conn, *l2_tick, timestamp);
    }

    void on_new_subscribe_rsp(
        const muduo::net::TcpConnectionPtr& conn,
        const utilities::MessagePtr& message,
        const muduo::Timestamp timestamp
    )
    {
        const auto& new_subscribe_rsp = std::dynamic_pointer_cast<types::NewSubscribeRsp>(message);
        if (new_subscribe_rsp != nullptr) {
            new_subscribe_rsp_callback(conn, *new_subscribe_rsp, timestamp);

            m_subscribed_symbols.clear();

            for (const auto& symbol : new_subscribe_rsp->subscribed_symbols())
                m_subscribed_symbols.insert(symbol);
        }
    }

    static void on_invalied_message(
        const muduo::net::TcpConnectionPtr&,
        const utilities::MessagePtr&,
        const muduo::Timestamp
    )
    {
        /// Do nothing here.
    }

private:
    L2SnapCallBackType l2_snap_callback;
    NewSubscribeRspCallBackType new_subscribe_rsp_callback;
    ConnectCallBackType connect_callback;
    DisconnectCallBackType disconnect_callback;

private:
    std::unordered_set<std::string> m_subscribed_symbols;

private:
    muduo::net::TcpConnectionPtr m_conn;
    utilities::ProtobufCodec m_codec;
    utilities::ProtobufDispatcher m_dispatcher;
    std::shared_ptr<muduo::net::EventLoop> m_loop;
    std::shared_ptr<muduo::net::TcpClient> m_client;
    std::future<void> m_event_loop_future;
    utilities::TickerTaper<int64_t> ticker_taper;
    std::string app_name;

private:
    std::atomic<bool> m_is_running;
};

} // namespace trade
