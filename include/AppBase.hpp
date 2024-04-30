#pragma once

#include <spdlog/spdlog.h>

namespace trade
{

class AppBase
{
public:
    AppBase(const std::string& name)
        : logger(spdlog::default_logger()->clone(name)) {};
    ~AppBase() = default;

public:
    mutable std::shared_ptr<spdlog::logger> logger;
};

} // namespace trade
