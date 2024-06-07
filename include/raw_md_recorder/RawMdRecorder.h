#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <fast-cpp-csv-parser/csv.h>

#include "AppBase.hpp"
#include "enums.pb.h"
#include "visibility.h"

namespace trade
{

class PUBLIC_API RawMdRecorder final: private AppBase<uint32_t>
{
public:
    RawMdRecorder(int argc, char* argv[]);
    ~RawMdRecorder() override = default;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    void odtd_receiver(const std::string& address, const std::string& interface_address);

    void write(const std::string& message, types::ExchangeType exchange_type);
    void write_sse(const std::string& message);
    void write_szse(const std::string& message);

    void new_sse_writer(const std::string& symbol);
    void new_szse_order_writer(const std::string& symbol);
    void new_szse_trade_writer(const std::string& symbol);

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running;
    std::atomic<int> m_exit_code;

private:
    std::unordered_map<std::string, std::ofstream> m_order_writers;
    std::unordered_map<std::string, std::ofstream> m_trade_writers;

private:
    static std::set<RawMdRecorder*> m_instances;
};

} // namespace trade
