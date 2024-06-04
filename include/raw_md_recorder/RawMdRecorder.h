#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <fast-cpp-csv-parser/csv.h>

#include "AppBase.hpp"
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
    void write(const std::string& raw_message);
    void new_order_writer(const std::string& symbol);
    void new_trade_writer(const std::string& symbol);

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    std::unordered_map<std::string, std::ofstream> m_order_writers;
    std::unordered_map<std::string, std::ofstream> m_trade_writers;

private:
    static std::set<RawMdRecorder*> m_instances;
};

} // namespace trade