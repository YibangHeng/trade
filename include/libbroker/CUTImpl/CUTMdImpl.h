#pragma once

#include "AppBase.hpp"
#include "OrderWrapper.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "third/ctp/ThostFtdcMdApi.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class CUTMdImpl final: private AppBase<int>,
                       private CThostFtdcMdSpi,
                       public liquibook::book::OrderBook<std::shared_ptr<OrderWrapper>>::TypedTradeListener
{
public:
    CUTMdImpl(
        std::shared_ptr<ConfigType> config,
        std::shared_ptr<holder::IHolder> holder,
        std::shared_ptr<reporter::IReporter> reporter
    );
    ~CUTMdImpl() override = default;

public:
    void subscribe(const std::unordered_set<std::string>& symbols);
    void unsubscribe(const std::unordered_set<std::string>& symbols);

public:
    void on_trade(
        const liquibook::book::OrderBook<OrderWrapperPtr>* book,
        liquibook::book::Quantity qty,
        liquibook::book::Price price
    ) override;

private:
    void odtd_receiver(const std::string& address);

private:
    static std::tuple<std::string, uint16_t> extract_address(const std::string& address);

private:
    using OrderBookPtr = std::shared_ptr<liquibook::book::OrderBook<OrderWrapperPtr>>;
    /// Symbol -> OrderBook.
    std::unordered_map<std::string, OrderBookPtr> books;

private:
    std::atomic<bool> is_running;
    std::thread* thread;

private:
    std::shared_ptr<holder::IHolder> m_holder;
    std::shared_ptr<reporter::IReporter> m_reporter;

private:
    /// Use self-managed login syncer for keeping trade running without market
    /// data.
    /// m_login_syncer always notifies success even if login fails.
    utilities::LoginSyncer m_login_syncer;
};

} // namespace trade::broker
