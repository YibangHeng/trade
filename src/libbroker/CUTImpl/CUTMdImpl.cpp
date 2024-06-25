#include "libbroker/CUTImpl/CUTMdImpl.h"
#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::broker::CUTMdImpl::CUTMdImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter
) : AppBase("CUTMdImpl", std::move(config)),
    m_holder(std::move(holder)),
    m_reporter(std::move(reporter))
{
}

void trade::broker::CUTMdImpl::subscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Wrong usage of CUTMdImpl::subscribe: symbols treated as multicast address and must be specified in config");
    }

    is_running = true;

    /// Split multicast addresses and start receiving in separate threads.
    auto addresses = config->get<std::string>("Server.MdAddresses");
    std::erase_if(addresses, [](const char c) { return std::isblank(c); });
    auto addresses_view = addresses | std::views::split(',');

    for (auto&& address_part : addresses_view) {
        std::string address(&*address_part.begin(), std::ranges::distance(address_part));

        threads.emplace_back(&CUTMdImpl::tick_receiver, this, address, config->get<std::string>("Server.InterfaceAddress"));

        logger->info("Subscribed to ODTD multicast address: {} at {}", address, config->get<std::string>("Server.InterfaceAddress"));
    }
}

void trade::broker::CUTMdImpl::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Wrong usage of CUTMdImpl::unsubscribe: symbols treated as multicast address and must be specified in config");
    }

    is_running = false;

    for (auto&& thread : threads) {
        thread.joinable() ? thread.join() : void();

        /// TODO: Find a way to get multicast address here.
        logger->info("Unsubscribed from ODTD multicast address: {} at {}", "unknown", config->get<std::string>("Server.InterfaceAddress"));
    }
}

void trade::broker::CUTMdImpl::tick_receiver(const std::string& address, const std::string& interface_address) const
{
    booker::Booker booker({}, m_reporter); /// TODO: Initialize tradable symbols here.

    const auto [multicast_ip, multicast_port] = utilities::AddressHelper::extract_address(address);
    utilities::MCClient<char[1024]> client(multicast_ip, multicast_port, interface_address);

    std::vector<u_char> buffer(1024);

    while (is_running) {
        const auto bytes_received = client.receive(buffer);

        booker::OrderTickPtr order_tick;
        booker::TradeTickPtr trade_tick;
        booker::L2TickPtr l2_tick;

        switch (bytes_received) {
        case sizeof(SSEHpfTick): order_tick = CUTCommonData::to_order_tick<SSEHpfTick>(buffer); break;
        case sizeof(SSEHpfL2Snap): l2_tick = CUTCommonData::to_l2_tick<SSEHpfL2Snap>(buffer); break;
        case sizeof(SZSEHpfOrderTick): order_tick = CUTCommonData::to_order_tick<SZSEHpfOrderTick>(buffer); break;
        case sizeof(SZSEHpfTradeTick): trade_tick = CUTCommonData::to_trade_tick<SZSEHpfTradeTick>(buffer); break;
        case sizeof(SZSEHpfL2Snap): l2_tick = CUTCommonData::to_l2_tick<SZSEHpfL2Snap>(buffer); break;
        default: break;
        }

        /// SSE has no raw trade tick. We tell it by order type.
        if (order_tick != nullptr && order_tick->order_type() == types::OrderType::fill) {
            trade_tick = CUTCommonData::x_ost_forward_to_trade_from_order(order_tick);
        }

        /// SZSE reports cancel orders as trade tick.
        /// In this case, forward it to order tick.
        if (trade_tick != nullptr && trade_tick->x_ost_szse_exe_type() == types::OrderType::cancel) {
            order_tick = CUTCommonData::x_ost_forward_to_order_from_trade(trade_tick);
        }

        if (order_tick != nullptr) {
            logger->debug("Received order tick: {}", utilities::ToJSON()(*order_tick));

            if (order_tick->exchange_time() >= 93000000) [[likely]]
                booker.switch_to_continuous_stage();

            booker.add(order_tick);

            m_reporter->exchange_order_tick_arrived(order_tick);
        }

        if (trade_tick != nullptr) {
            logger->debug("Received trade tick: {}", utilities::ToJSON()(*trade_tick));

            booker.trade(trade_tick);

            m_reporter->exchange_trade_tick_arrived(trade_tick);
        }

        if (l2_tick != nullptr) {
            logger->debug("Received l2 tick: {}", utilities::ToJSON()(*l2_tick));

            booker.l2(l2_tick);

            m_reporter->exchange_l2_tick_arrived(l2_tick);
        }
    }
}
