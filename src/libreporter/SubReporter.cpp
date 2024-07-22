#include <third/muduo/muduo/base/Logging.h>
#include <third/muduo/muduo/net/InetAddress.h>

#include "libreporter/SubReporter.h"

#include <future>

trade::reporter::SubReporter::SubReporter(const int64_t port, std::shared_ptr<IReporter> outside)
    : AppBase("SubReporter"),
      m_codec([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& receive_time) { m_dispatcher.on_protobuf_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(receive_time)); }),
      m_dispatcher([this]<typename ConnType, typename MessageType, typename TimestampType>(ConnType&& conn, MessageType&& message, TimestampType&& timestamp) { on_invalid_message(std::forward<ConnType>(conn), std::forward<MessageType>(message), std::forward<TimestampType>(timestamp)); }),
      m_outside(std::move(outside))
{
    muduo::Logger::setLogLevel(muduo::Logger::NUM_LOG_LEVELS);

    m_dispatcher.register_message_callback<types::NewSubscribeReq>([this](const muduo::net::TcpConnectionPtr& conn, const utilities::MessagePtr& message, const muduo::Timestamp timestamp) { on_new_subscribe_req(conn, message, timestamp); });

    /// Start event loop.
    m_event_loop_future = std::async(std::launch::async, [this, port] {
        m_loop   = std::make_shared<muduo::net::EventLoop>();
        m_server = std::make_shared<muduo::net::TcpServer>(m_loop.get(), muduo::net::InetAddress(port), app_name());

        m_server->setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn) { on_connected(conn); });
        m_server->setMessageCallback([this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, const muduo::Timestamp receive_time) { m_codec.on_message(conn, buf, receive_time); });

        m_server->start();

        /// Enter event loop until quit.
        m_loop->loop();

        m_server.reset();
        m_loop.reset();
    });
}

trade::reporter::SubReporter::~SubReporter()
{
    m_loop->quit();
    m_event_loop_future.wait();
}

void trade::reporter::SubReporter::exchange_l2_tick_arrived(const std::shared_ptr<types::L2Tick> l2_tick)
{
    std::lock_guard lock(m_app_id_to_symbols_mutex);

    for (const auto& [conn, symbols] : m_app_id_to_symbols) {
        if (symbols.contains(l2_tick->symbol())) {
            utilities::ProtobufCodec::send(conn, *l2_tick);
        }
    }

    m_outside->l2_tick_generated(l2_tick);
}

void trade::reporter::SubReporter::on_connected(const muduo::net::TcpConnectionPtr& conn)
{
    std::lock_guard lock(m_app_id_to_symbols_mutex);

    if (conn->connected()) {
        m_app_id_to_symbols.emplace(conn, std::unordered_set<std::string> {});
        logger->info("{}({}) connected. Now we have {} subscriptions", conn->name(), conn->peerAddress().toIpPort(), m_app_id_to_symbols.size());
    }
    else {
        m_app_id_to_symbols.erase(conn);
        logger->info("{}({}) disconnected. Now we have {} subscriptions", conn->name(), conn->peerAddress().toIpPort(), m_app_id_to_symbols.size());
    }
}

void trade::reporter::SubReporter::on_new_subscribe_req(
    const muduo::net::TcpConnectionPtr& conn,
    const utilities::MessagePtr& message,
    muduo::Timestamp
)
{
    const auto new_subscribe_req = std::dynamic_pointer_cast<types::NewSubscribeReq>(message);

    if (new_subscribe_req == nullptr) {
        logger->warn("Invalid message received from {}", conn->peerAddress().toIpPort());
        return;
    }

    update_subscripted_symbols(conn, *new_subscribe_req);

    types::NewSubscribeRsp new_subscribe_rsp;

    new_subscribe_rsp.set_request_id(new_subscribe_req->has_request_id() ? new_subscribe_req->request_id() : ticker_taper());

    {
        std::lock_guard lock(m_app_id_to_symbols_mutex);

        for (const auto& symbol : m_app_id_to_symbols[conn])
            new_subscribe_rsp.add_subscribed_symbol(symbol);
    }

    utilities::ProtobufCodec::send(conn, new_subscribe_rsp);
}

void trade::reporter::SubReporter::on_invalid_message(
    const muduo::net::TcpConnectionPtr& conn,
    const utilities::MessagePtr&,
    muduo::Timestamp
) const
{
    logger->warn("Invalid message received from {}", conn->peerAddress().toIpPort());
}

void trade::reporter::SubReporter::update_subscripted_symbols(
    const muduo::net::TcpConnectionPtr& conn,
    const types::NewSubscribeReq& new_subscribe_req
)
{
    std::lock_guard lock(m_app_id_to_symbols_mutex);

    auto& subscripted_symbol_set = m_app_id_to_symbols[conn];

    for (const auto& symbol : new_subscribe_req.symbols_subscribe()) {
        if (symbol == "*") {
            subscripted_symbol_set.clear();
            subscripted_symbol_set.insert("*");
            break;
        }

        subscripted_symbol_set.insert(symbol);
    }

    for (const auto& symbol : new_subscribe_req.symbol_unsubscribe()) {
        if (symbol == "*") {
            subscripted_symbol_set.clear();
            break;
        }

        subscripted_symbol_set.erase(symbol);
    }
}