#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <set>

#include "AppBase.hpp"
#include "visibility.h"

namespace trade
{

class PUBLIC_API Trade final: private AppBase<>
{
public:
    Trade(int argc, char* argv[]);
    ~Trade() override;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    int network_events() const;

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<Trade*> m_instances;
};

} // namespace trade
