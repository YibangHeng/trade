#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <fast-cpp-csv-parser/csv.h>

#include "AppBase.hpp"
#include "SubReporterClientImpl.hpp"
#include "auxiliaries/sub_reporter_client/SubReporterClient.h"
#include "networks.pb.h"
#include "utilities/ProtobufDispatcher.hpp"
#include "visibility.h"

namespace trade
{

class TD_PUBLIC_API SubReporterClient final: private AppBase<uint32_t>
{
public:
    SubReporterClient(int argc, char* argv[]);
    ~SubReporterClient() override = default;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    void on_l2_snap_arrived(
        const muduo::net::TcpConnectionPtr& conn,
        const types::ExchangeL2Snap& exchange_tick_snap,
        muduo::Timestamp timestamp
    ) const;

    void on_new_subscribe_rsp(
        const muduo::net::TcpConnectionPtr& conn,
        const types::NewSubscribeRsp& new_subscribe_rsp,
        muduo::Timestamp timestamp
    ) const;

    void on_connected(const muduo::net::TcpConnectionPtr& conn);

    void on_disconnected(const muduo::net::TcpConnectionPtr& conn) const;

private:
    boost::program_options::variables_map m_arguments;

private:
    std::shared_ptr<SubReporterClientImpl> m_sub_reporter_client_impl;

private:
    std::atomic<bool> m_is_running;
    std::atomic<int> m_exit_code;

private:
    static std::set<SubReporterClient*> m_instances;
};

} // namespace trade
