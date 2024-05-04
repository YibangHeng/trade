#pragma once

#include <boost/core/noncopyable.hpp>
#include <spdlog/spdlog.h>

#include "utilities/ConfigHelper.hpp"

namespace trade
{

class AppBase: boost::noncopyable
{
public:
    explicit AppBase(
        const std::string& name,
        const std::string& config_path = ""
    ) : logger(spdlog::default_logger()->clone(name)),
        config(!config_path.empty() ? std::make_shared<utilities::INIConfig>(config_path) : nullptr)
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
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<utilities::INIConfig> config;
};

} // namespace trade
