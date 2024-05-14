#pragma once

#include <atomic>
#include <boost/program_options.hpp>
#include <set>
#include <zmq.h>

#include "AppBase.hpp"
#include "visibility.h"

namespace trade
{

struct ZMQContextPtrDeleter {
    void operator()(void* ptr) const
    {
        zmq_ctx_destroy(ptr);
    }
};

using ZMQContextPtr = std::unique_ptr<void, ZMQContextPtrDeleter>;

struct ZMQSocketPtrDeleter {
    void operator()(void* ptr) const
    {
        zmq_close(ptr);
    }
};

using ZMQSocketPtr = std::unique_ptr<void, ZMQSocketPtrDeleter>;

class PUBLIC_API Trade final: private AppBase<>
{
public:
    Trade(int argc, char* argv[]);
    ~Trade() override;

public:
    int run();
    int stop();
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    void init_zmq(const std::string& socket);
    int network_events() const;

private:
    boost::program_options::variables_map m_arguments;

private:
    ZMQContextPtr zmq_context;
    ZMQSocketPtr zmq_socket;
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<Trade*> m_instances;
};

} // namespace trade
