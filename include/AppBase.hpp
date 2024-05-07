#pragma once

#include <boost/core/noncopyable.hpp>
#include <spdlog/spdlog.h>
#include <utility>

#include "utilities/ConfigHelper.hpp"
#include "utilities/SnowFlaker.hpp"
#include "utilities/TickerTaper.hpp"

namespace trade
{

/// AppBase, which contains all the common functionalities (logger, config,
/// etc), is the base class for all classes.
template<typename TickerTaperT = int64_t, utilities::ConfigFileType ConfigFileType = utilities::ConfigFileType::INI>
class AppBase: private boost::noncopyable
{
protected:
    using ConfigType = utilities::ConfigHelper<ConfigFileType>;

public:
    explicit AppBase(
        const std::string& name,
        const std::string& config_path = ""
    ) : config(!config_path.empty() ? std::make_shared<ConfigType>(config_path) : nullptr),
        logger(spdlog::default_logger()->clone(name))
    {
        /// TODO: Make it configurable.
        snow_flaker.init(1, 1);
    }

    AppBase(
        const std::string& name,
        const std::shared_ptr<ConfigType> config
    ) : config(std::move(config)), logger(spdlog::default_logger()->clone(name))
    {}

    virtual ~AppBase() = default;

public:
    /// Reset the configuration to the given path.
    /// @param config_path Path to the new configuration.
    /// @throws boost::property_tree::ini_parser_error If the configuration file could not be read.
    void reset_config(const std::string& config_path) noexcept(false)
    {
        config = std::make_shared<utilities::INIConfig>(config_path);
    }

    /// Create a new seq/unique id pair, which can be used in the future.
    /// @return The new seq/unique id pair.
    auto new_id_pair()
    {
        std::lock_guard lock(m_id_map_mutex);

        const auto seq_id    = ticker_taper();
        const auto unique_id = snow_flaker();

        m_id_map.emplace(seq_id, unique_id);

        return std::make_tuple(seq_id, unique_id);
    }

    /// Get the unique id by the given sequence id.
    /// @param seq_id The sequence id.
    /// @return The unique id. If the given sequence id is not found, return INVALID_ID.
    auto get_by_seq_id(const TickerTaperT& seq_id)
    {
        std::lock_guard lock(m_id_map_mutex);

        return m_id_map.contains(seq_id) ? m_id_map[seq_id] : INVALID_ID;
    }

    /// Erase the unique id by the given sequence id.
    void earse_by_seq_id(const TickerTaperT& seq_id)
    {
        std::lock_guard lock(m_id_map_mutex);

        m_id_map.erase(seq_id);
    }

public:
    std::shared_ptr<ConfigType> config;
    std::shared_ptr<spdlog::logger> logger;
    utilities::TickerTaper<TickerTaperT> ticker_taper;
    utilities::SnowFlaker<946684800000l> snow_flaker;

private:
    std::mutex m_id_map_mutex;
    std::unordered_map<decltype(ticker_taper()), decltype(snow_flaker())> m_id_map;
};

} // namespace trade
