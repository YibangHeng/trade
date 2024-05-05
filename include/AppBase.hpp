#pragma once

#include <boost/core/noncopyable.hpp>
#include <spdlog/spdlog.h>
#include <utility>

#include "utilities/ConfigHelper.hpp"
#include "utilities/TickerTaper.hpp"

namespace trade
{

/// AppBase, which contains all the common functionalities (logger, config,
/// etc), is the base class for all classes.
template<typename TickerTaperT = int64_t, utilities::ConfigFileType ConfigFileType = utilities::ConfigFileType::INI>
class AppBase: boost::noncopyable
{
private:
    using ConfigType = utilities::ConfigHelper<ConfigFileType>;

public:
    explicit AppBase(
        const std::string& name,
        const std::string& config_path = ""
    ) : config(!config_path.empty() ? std::make_shared<ConfigType>(config_path) : nullptr),
        logger(spdlog::default_logger()->clone(name))
    {}

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

public:
    std::shared_ptr<ConfigType> config;
    std::shared_ptr<spdlog::logger> logger;
    utilities::TickerTaper<TickerTaperT> ticker_taper;
};

} // namespace trade
