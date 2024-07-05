#pragma once

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>

#include "AppBase.hpp"
#include "enums.pb.h"
#include "libbooker/Booker.h"
#include "libbooker/BookerCommonData.h"
#include "libreporter/IReporter.hpp"
#include "visibility.h"

namespace trade
{

struct StdTick {
    std::string symbol;
    int64_t ask_unique_id;
    int64_t bid_unique_id;
    types::OrderType order_type;
    double price;
    int64_t quantity;
    int64_t time;
};

class PUBLIC_API OfflineBooker final: private AppBase<>
{
public:
    OfflineBooker(int argc, char* argv[]);
    ~OfflineBooker() override = default;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    void load_tick(const std::string& path);
    void booker(StdTick* std_tick) const;

private:
    [[nodiscard]] static types::OrderType to_order_type(char order_type);
    [[nodiscard]] static booker::OrderTickPtr to_order_tick(StdTick* std_tick);
    [[nodiscard]] static booker::TradeTickPtr to_trade_tick(StdTick* std_tick);
    [[nodiscard]] static types::SideType to_side(int64_t ask_unique_id, int64_t bid_unique_id);
    [[nodiscard]] static int64_t to_exchange_time(int64_t time);

private:
    boost::lockfree::spsc_queue<StdTick*, boost::lockfree::capacity<1024>> m_pairs;
    std::shared_ptr<booker::Booker> m_booker;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<OfflineBooker*> m_instances;
};

} // namespace trade
