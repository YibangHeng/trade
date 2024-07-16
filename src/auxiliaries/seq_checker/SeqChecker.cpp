#include <csignal>
#include <fast-cpp-csv-parser/csv.h>
#include <filesystem>
#include <fmt/ranges.h>
#include <future>
#include <iostream>
#include <ranges>

#include "auxiliaries/seq_checker/SeqChecker.h"
#include "enums.pb.h"
#include "info.h"
#include "libbooker/BookerCommonData.h"
#include "utilities/MakeAssignable.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/SeqChecker.hpp"

trade::SeqChecker::SeqChecker(const int argc, char* argv[])
    : AppBase("seq_checker")
{
    m_instances.emplace(this);
    m_is_running = argv_parse(argc, argv);
}

int trade::SeqChecker::run()
{
    if (!m_is_running) {
        return m_exit_code;
    }

    const std::filesystem::path folder(m_arguments["sse-tick-folder"].as<std::string>());

    if (!is_directory(folder)) {
        logger->error("{} is not a directory", folder.string());
        return EXIT_FAILURE;
    }

    const auto future = std::async(std::launch::async, [this, &folder] {
        /// Load all seq per channel.
        for (const auto& file : std::filesystem::directory_iterator(folder)) {
            if (file.is_regular_file() && file.path().filename() == "sse-tick.csv") {
                load_seq(file.path().string());
                logger->info("Loaded file {}", file.path().string());

                if (!m_is_running)
                    return;
            }
        }
    });

    std::map<uint64_t, std::vector<uint64_t>> seqs;

    while (true) {
        ChannelSeqPair channel_seq_pair {};

        if (m_pairs.pop(channel_seq_pair))
            seqs[channel_seq_pair.channel_id].push_back(channel_seq_pair.seq);
        else if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            break;

        if (!m_is_running)
            return EXIT_FAILURE;
    }

    logger->info("Loaded {} channels: {}", seqs.size(), seqs | std::views::keys);

    report(seqs);

    return m_exit_code;
}

int trade::SeqChecker::stop(int signal)
{
    spdlog::info("App exiting since received signal {}", signal);

    m_is_running = false;

    return 0;
}

void trade::SeqChecker::signal(const int signal)
{
    switch (signal) {
    case SIGINT:
    case SIGTERM: {
        for (const auto& instance : m_instances) {
            instance->stop(signal);
        }

        break;
    }
    default: {
        spdlog::info("Signal {} omitted", signal);
    }
    }
}

bool trade::SeqChecker::argv_parse(const int argc, char* argv[])
{
    boost::program_options::options_description desc("Allowed options");

    /// Help and version.
    desc.add_options()("help,h", "print help message");
    desc.add_options()("version,v", "print version string and exit");

    desc.add_options()("debug,d", "enable debug output");

    /// Order/Trade ticks folder.
    desc.add_options()("sse-tick-folder", boost::program_options::value<std::string>()->default_value("./data/sse_order_and_trade"), "SSE tick folder");

    try {
        store(parse_command_line(argc, argv, desc), m_arguments);
    }
    catch (const boost::wrapexcept<boost::program_options::unknown_option>& e) {
        std::cout << e.what() << std::endl;
        m_exit_code = EXIT_FAILURE;
        return false;
    }

    notify(m_arguments);

/// contains() is not support on Windows platforms.
#if WIN32
    #define contains(s) count(s) > 1
#endif

    if (m_arguments.contains("help")) {
        std::cout << desc;
        return false;
    }

    if (m_arguments.contains("version")) {
        std::cout << fmt::format("{} {}", app_name(), trade_VERSION) << std::endl;
        return false;
    }

    if (m_arguments.contains("debug")) {
        spdlog::set_level(spdlog::level::debug);
        this->logger->set_level(spdlog::level::debug);
    }

#if WIN32
    #undef contains
#endif

    return true;
}

void trade::SeqChecker::load_seq(const std::string& path)
{
    io::CSVReader<2> in(path);

    in.read_header(
        io::ignore_extra_column,
        "channel_id",
        "tick_index"
    );

    uint64_t channel_id;
    uint64_t tick_index;

    while (m_is_running && in.read_row(channel_id, tick_index)) {
        while (!m_pairs.push(ChannelSeqPair(channel_id, tick_index)))
            logger->debug("Adding tick {:>8} for channel {:<3}", tick_index, channel_id);
        logger->debug("Added tick {:>8} for channel {:<3}", tick_index, channel_id);
    }
}

void trade::SeqChecker::report(const std::map<uint64_t, std::vector<uint64_t>>& seqs)
{
    std::cout << std::endl
              << "=== Report =============="
              << std::endl
              << std::endl;

    for (const auto& [channel, seqs] : seqs) {
        const uint64_t min = *std::ranges::min_element(seqs);
        const uint64_t max = *std::ranges::max_element(seqs);

        /// Do not use seqs.size() here since we treat seq as index.
        std::vector<uint8_t> seq_count(max + 1, 0);

        for (const auto& seq : seqs) {
            seq_count[seq]++;
        }

        print_result(channel, min, max, seq_count);
    }
}

void trade::SeqChecker::print_result(
    uint64_t channel,
    uint64_t min, uint64_t max,
    const std::vector<uint8_t>& seq_count
)
{
    std::vector<uint64_t> missing;
    std::vector<uint64_t> duplication;

    for (size_t index = min; index < max; index++) {
        if (seq_count[index] == 0) {
            missing.push_back(index);
        }
        else if (seq_count[index] > 1) {
            duplication.push_back(index);
        }
    }

    std::cout << fmt::format(
        "For channel {}:\n"
        "    min {:<6} max {:<12}\n"
        "    mis ({}): {}\n"
        "    dup ({}): {}\n",
        channel,
        min, max,
        missing.size(), missing,
        duplication.size(), duplication
    ) << std::endl;
}

std::set<trade::SeqChecker*> trade::SeqChecker::m_instances;
