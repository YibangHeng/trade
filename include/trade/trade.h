#pragma once

#include <atomic>
#include <boost/noncopyable.hpp>
#include <set>

#include "AppBase.hpp"

namespace trade
{

class Trade: private boost::noncopyable, private AppBase
{
public:
    Trade(int argc, char* argv[]);
    ~Trade();

public:
    int run() const;
    int stop();
    static void signal(int signal);

private:
    std::atomic<bool> is_running = false;

private:
    static std::set<Trade*> instances;
};

} // namespace trade
