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

    void write(const std::vector<u_char>& message, size_t bytes_received);

    void write_sse_tick(const std::vector<u_char>& message);
    void write_sse_l2snap(const std::vector<u_char>& message);

    void write_szse_order_tick(const std::vector<u_char>& message);
    void write_szse_trade_tick(const std::vector<u_char>& message);
    void write_szse_l2snap(const std::vector<u_char>& message);

    void new_sse_tick_writer(const std::string& symbol);
    void new_sse_l2_snap_writer(const std::string& symbol);

    void new_szse_order_writer(const std::string& symbol);
    void new_szse_trade_writer(const std::string& symbol);
    void new_szse_l2_snap_writer(const std::string& symbol);

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running;
    std::atomic<int> m_exit_code;

    /// Symbol -> ofstream.
private:
    std::unordered_map<std::string, std::ofstream> m_sse_tick_writers;
    std::unordered_map<std::string, std::ofstream> m_sse_l2_snap_writers;

    std::unordered_map<std::string, std::ofstream> m_szse_order_writers;
    std::unordered_map<std::string, std::ofstream> m_szse_trade_writers;
    std::unordered_map<std::string, std::ofstream> m_szse_l2_snap_writers;

private:
    static std::set<RawMdRecorder*> m_instances;
};

} // namespace trade
