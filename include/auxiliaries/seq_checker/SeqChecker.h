#pragma once

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>

#include "AppBase.hpp"
#include "visibility.h"

namespace trade
{

struct ChannelSeqPair {
    uint64_t channel_id;
    uint64_t seq;
};

class TD_PUBLIC_API SeqChecker final: private AppBase<>
{
public:
    SeqChecker(int argc, char* argv[]);
    ~SeqChecker() override = default;

public:
    int run();
    int stop(int signal);
    static void signal(int signal);

private:
    bool argv_parse(int argc, char* argv[]);

private:
    void load_seq(const std::string& path);
    static void report(const std::map<uint64_t, std::vector<uint64_t>>& seqs);
    static void print_result(
        uint64_t channel,
        uint64_t min, uint64_t max,
        const std::vector<uint8_t>& seq_count
    );

private:
    boost::lockfree::spsc_queue<ChannelSeqPair, boost::lockfree::capacity<1024>> m_pairs;

private:
    boost::program_options::variables_map m_arguments;

private:
    std::atomic<bool> m_is_running = false;
    std::atomic<int> m_exit_code   = 0;

private:
    static std::set<SeqChecker*> m_instances;
};

} // namespace trade
