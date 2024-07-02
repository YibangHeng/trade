#pragma once

#include <atomic>
#include <boost/program_options.hpp>

#include "AppBase.hpp"
#include "libbroker/CUTImpl/RawStructure.h"
#include "visibility.h"

namespace trade
{

class PUBLIC_API OSTMdServer final: private AppBase<>
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
    const broker::SSEHpfTick& emit_sse_tick(const std::string& path);
    std::vector<broker::SSEHpfTick> read_sse_tick(const std::string& path) const;

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<OSTMdServer*> m_instances;
};

} // namespace trade
