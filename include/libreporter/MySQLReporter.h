#pragma once

#include <mysqlx/xdevapi.h>

#include "AppBase.hpp"
#include "IReporter.hpp"
#include "NopReporter.hpp"

namespace trade::reporter
{

class PUBLIC_API MySQLReporter final: private AppBase<>, public NopReporter
{
public:
    MySQLReporter(
        const std::string& url,
        const std::string& user,
        const std::string& password,
        const std::string& database,
        const std::string& table,
        const std::shared_ptr<IReporter>& outside = std::make_shared<NopReporter>()
    );
    ~MySQLReporter() override;

    /// Market data.
public:
    void ranged_tick_generated(std::shared_ptr<types::RangedTick> ranged_tick) override;

private:
    std::shared_ptr<mysqlx::Session> m_sess;
    std::shared_ptr<mysqlx::Schema> m_schema;
    std::shared_ptr<mysqlx::Table> m_table;

private:
    std::shared_ptr<IReporter> m_outside;
};

} // namespace trade::reporter