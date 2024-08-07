#pragma once

#include <spdlog/spdlog.h>
#include <utility>

#include "AppBase.hpp"
#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

class TD_PUBLIC_API CSVReporter final: private AppBase<>, public NopReporter
{
public:
    explicit CSVReporter(
        std::string output_folder,
        const std::shared_ptr<IReporter>& outside = std::make_shared<NopReporter>()
    )
        : AppBase("CSVReporter"),
          NopReporter(outside),
          m_output_folder(std::move(output_folder)),
          m_outside(outside)
    {
    }
    ~CSVReporter() override;

    /// Market data.
public:
    void l2_tick_generated(std::shared_ptr<types::GeneratedL2Tick> generated_l2_tick) override;
    void ranged_tick_generated(std::shared_ptr<types::RangedTick> ranged_tick) override;

private:
    void new_l2_tick_writer(const std::string& symbol);
    void new_ranged_tick_writer(const std::string& symbol);

private:
    std::string m_output_folder;
    /// Symbol -> ofstream.
    std::unordered_map<std::string, std::ofstream> m_l2_tick_writers;
    std::unordered_map<std::string, std::ofstream> m_ranged_tick_writers;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter