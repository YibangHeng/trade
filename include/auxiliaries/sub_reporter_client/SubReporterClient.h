#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <fast-cpp-csv-parser/csv.h>
#include <future>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>

#include "AppBase.hpp"
#include "networks.pb.h"
#include "utilities/ProtobufDispatcher.hpp"
#include "visibility.h"

namespace trade
{

class PUBLIC_API SubReporterClient final: private AppBase<uint32_t>
{
public:
    SubReporterClient(int argc, char* argv[]);
    ~SubReporterClient() override;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    void on_connected(const muduo::net::TcpConnectionPtr& conn)
    {
        if (conn->connected()) {
            types::NewSubscribeReq req;

            req.set_request_id(ticker_taper());
            req.set_app_name(app_name());
            req.add_symbols_subscribe("600875");
            req.add_symbol_unsubscribe("000001");

            utilities::ProtobufCodec::send(conn, req);
        }
        else {
            logger->error("Connection lost");
        }
    }

    void on_new_subscribe_rsp(
        const muduo::net::TcpConnectionPtr& conn,
        const utilities::MessagePtr& message,
        muduo::Timestamp
    )
    {
    }

    void on_invalied_message(const muduo::net::TcpConnectionPtr& conn, const utilities::MessagePtr&, muduo::Timestamp) const
    {
        logger->warn("Invalid message received from {}", conn->peerAddress().toIpPort());
    }

private:
    boost::program_options::variables_map m_arguments;

private:
    utilities::ProtobufCodec m_codec;
    utilities::ProtobufDispatcher m_dispatcher;
    std::shared_ptr<muduo::net::EventLoop> m_loop;
    std::shared_ptr<muduo::net::TcpClient> m_client;
    std::future<void> m_event_loop_future;

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;

private:
    std::atomic<bool> m_is_running;
    std::atomic<int> m_exit_code;

private:
    static std::set<SubReporterClient*> m_instances;
};

} // namespace trade
