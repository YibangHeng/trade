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
    ConfigType m_config;
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
        read_ini(configPath, m_config);
    }

public:
    /// Retrieves a value from the configuration based on the given key.
    /// @tparam Rt Expected return type.
    /// @param key Path to find the value in the configuration.
    /// @return Value retrieved from the configuration.
    template<typename Rt>
    Rt get(const std::string& key) const { return m_config.get<Rt>(key); }
    template<typename Rt>
    Rt get(const std::string& key, Rt default_value) const
    {
        try {
            return m_config.get<Rt>(key);
        }
        catch (const boost::property_tree::ptree_bad_path&) {
            return default_value;
        }
    }
};

template<>
class ConfigHelper<ConfigFileType::JSON>: private ConfigHelperBase
{
public:
    explicit ConfigHelper(const std::string& config_path)
    {
        read_json(config_path, m_config);
    }

public:
    /// Retrieves a value from the configuration based on the given key.
    /// @tparam Rt Expected return type.
    /// @param key Path to find the value in the configuration.
    /// @return Value retrieved from the configuration.
    template<typename Rt>
    Rt get(const std::string& key) const { return m_config.get<Rt>(key); }
    template<typename Rt>
    Rt get(const std::string& key, Rt default_value) const
    {
        try {
            return m_config.get<Rt>(key);
        }
        catch (const boost::property_tree::ptree_bad_path&) {
            return default_value;
        }
    }
};

template<>
class ConfigHelper<ConfigFileType::XML>: private ConfigHelperBase
{
public:
    explicit ConfigHelper(const std::string& config_path)
    {
        read_xml(config_path, m_config);
    }

public:
    /// Retrieves a value from the configuration based on the given key.
    /// @tparam Rt Expected return type.
    /// @param key Path to find the value in the configuration.
    /// @return Value retrieved from the configuration.
    template<typename Rt>
    Rt get(const std::string& key) const { return m_config.get<Rt>(key); }
    template<typename Rt>
    Rt get(const std::string& key, Rt default_value) const
    {
        try {
            return m_config.get<Rt>(key);
        }
        catch (const boost::property_tree::ptree_bad_path&) {
            return default_value;
        }
    }
};

/// Type alias for convenience.
using INIConfig  = ConfigHelper<ConfigFileType::INI>;
using JSONConfig = ConfigHelper<ConfigFileType::JSON>;
using XMLConfig  = ConfigHelper<ConfigFileType::XML>;

} // namespace trade::utilities