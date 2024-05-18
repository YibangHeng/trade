#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <set>

#include "AppBase.hpp"
#include "libbroker/IBroker.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
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
    std::shared_ptr<reporter::IReporter> m_reporter;
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<broker::IBroker> m_broker;

private:
    static std::set<Trade*> m_instances;
};

} // namespace trade
