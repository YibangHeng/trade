#pragma once

#include <third/muduo/muduo/net/EventLoop.h>
#include <third/muduo/muduo/net/TcpConnection.h>
#include <third/muduo/muduo/net/TcpServer.h>

#include "AppBase.hpp"
#include "IReporter.hpp"
#include "NopReporter.hpp"
#include "utilities/ProtobufDispatcher.hpp"

#include <future>

namespace trade::reporter
{

class PUBLIC_API SubReporter final: private AppBase<>, public NopReporter
{
public:
    explicit SubReporter(
        int64_t port,
        const std::shared_ptr<IReporter>& outside = std::make_shared<NopReporter>()
    );
    ~SubReporter() override;

    /// Market data.
public:
    void exchange_l2_tick_arrived(std::shared_ptr<types::ExchangeL2Snap> exchange_l2_snap) override;

private:
    void on_connected(const muduo::net::TcpConnectionPtr& conn);
    void on_new_subscribe_req(
        const muduo::net::TcpConnectionPtr& conn,
        const utilities::MessagePtr& message,
        muduo::Timestamp
    );
    void on_invalid_message(
        const muduo::net::TcpConnectionPtr& conn,
        const utilities::MessagePtr& message,
        muduo::Timestamp
    ) const;

private:
    void update_subscripted_symbols(
        const muduo::net::TcpConnectionPtr& conn,
        const types::NewSubscribeReq& new_subscribe_req
    );

private:
    std::unordered_map<std::string, std::shared_ptr<types::ExchangeL2Snap>> m_last_data;

private:
    utilities::ProtobufCodec m_codec;
    utilities::ProtobufDispatcher m_dispatcher;
    std::shared_ptr<muduo::net::EventLoop> m_loop;
    std::shared_ptr<muduo::net::TcpServer> m_server;
    std::future<void> m_event_loop_future;

private:
    /// Conn -> set of subscribed symbols (not including what subscribed to all symbols).
    std::unordered_map<muduo::net::TcpConnectionPtr, std::unordered_set<std::string>> m_app_id_to_symbols;
    std::mutex m_app_id_to_symbols_mutex;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter