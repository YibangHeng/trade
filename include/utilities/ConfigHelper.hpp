#pragma once

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace trade::utilities
{

/// Supported config file types.
enum class ConfigFileType
{
    invalid = 0,
    INI     = 1,
    JSON    = 2,
    XML     = 3
};

/// Base class for ConfigHelper.
/// Provides a common ConfigType for all ConfigHelper specializations.
class ConfigHelperBase
{
protected:
    using ConfigType = boost::property_tree::ptree;
    ConfigType _config;
};

/// Helper class for parsing config files.
/// Do not use directly. See specializations.
template<ConfigFileType>
class ConfigHelper
{
public:
    ConfigHelper() = delete;
};

template<>
class ConfigHelper<ConfigFileType::INI>: private ConfigHelperBase
{
public:
    explicit ConfigHelper(const std::string& configPath)
    {
        read_ini(configPath, _config);
    }

public:
    /// Retrieves a value from the configuration based on the given key.
    /// @tparam Rt Expected return type.
    /// @param key Path to find the value in the configuration.
    /// @return Value retrieved from the configuration.
    template<typename Rt>
    Rt get(const std::string& key) const { return _config.get<Rt>(key); }
    template<typename Rt>
    Rt get(const std::string& key, Rt defaultValue) const
    {
        try {
            return _config.get<Rt>(key);
        }
        catch (const boost::property_tree::ptree_bad_path&) {
            return defaultValue;
        }
    }
};

template<>
class ConfigHelper<ConfigFileType::JSON>: private ConfigHelperBase
{
public:
    explicit ConfigHelper(const std::string& configPath)
    {
        read_json(configPath, _config);
    }

public:
    /// Retrieves a value from the configuration based on the given key.
    /// @tparam Rt Expected return type.
    /// @param key Path to find the value in the configuration.
    /// @return Value retrieved from the configuration.
    template<typename Rt>
    Rt get(const std::string& key) const { return _config.get<Rt>(key); }
    template<typename Rt>
    Rt get(const std::string& key, Rt defaultValue) const
    {
        try {
            return _config.get<Rt>(key);
        }
        catch (const boost::property_tree::ptree_bad_path&) {
            return defaultValue;
        }
    }
};

template<>
class ConfigHelper<ConfigFileType::XML>: private ConfigHelperBase
{
public:
    explicit ConfigHelper(const std::string& configPath)
    {
        read_xml(configPath, _config);
    }

public:
    /// Retrieves a value from the configuration based on the given key.
    /// @tparam Rt Expected return type.
    /// @param key Path to find the value in the configuration.
    /// @return Value retrieved from the configuration.
    template<typename Rt>
    Rt get(const std::string& key) const { return _config.get<Rt>(key); }
    template<typename Rt>
    Rt get(const std::string& key, Rt defaultValue) const
    {
        try {
            return _config.get<Rt>(key);
        }
        catch (const boost::property_tree::ptree_bad_path&) {
            return defaultValue;
        }
    }
};

/// Type alias for convenience.
using INIConfig  = ConfigHelper<ConfigFileType::INI>;
using JSONConfig = ConfigHelper<ConfigFileType::JSON>;
using XMLConfig  = ConfigHelper<ConfigFileType::XML>;

} // namespace trade::utilities