#pragma once

#include <atomic>
#include <boost/program_options.hpp>

#include "AppBase.hpp"
#include "libbroker/CUTImpl/CUTCommonData.h"
#include "visibility.h"

namespace trade
{

class PUBLIC_API OSTMdServer final: private AppBase<uint32_t>
{
public:
    OSTMdServer(int argc, char* argv[]);
    ~OSTMdServer() override = default;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    const broker::sze_hpf_order_pkt& emit_od_tick(const std::string& path);
    std::vector<broker::sze_hpf_order_pkt> read_od(const std::string& path);

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<OSTMdServer*> m_instances;
};

} // namespace trade
