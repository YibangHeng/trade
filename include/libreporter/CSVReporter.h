#pragma once

#include <spdlog/spdlog.h>
#include <utility>

#include "AppBase.hpp"
#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

class PUBLIC_API CSVReporter final: private AppBase<>, public NopReporter
{
public:
    explicit CSVReporter(
        std::string output_folder,
        std::shared_ptr<IReporter> outside = std::make_shared<NopReporter>()
    )
        : AppBase("CSVReporter"),
          m_output_folder(std::move(output_folder)),
          m_outside(std::move(outside))
    {
    }
    ~CSVReporter() override = default;

    /// Market data.
public:
    void md_trade_generated(std::shared_ptr<types::MdTrade> md_trade) override;

private:
    void new_md_trade_writer(const std::string& symbol);

private:
    std::string m_output_folder;
    /// Symbol -> ofstream.
    std::unordered_map<std::string, std::ofstream> m_md_trade_writers;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter