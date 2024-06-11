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
    booker({}, reporter), /// TODO: Initialize tradable symbols here.
    m_holder(std::move(holder)),
    m_reporter(std::move(reporter))
{
}

void trade::broker::CUTMdImpl::subscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Wrong usage of subscribe of CUT: symbols treated as multicast address and must be specified in config");
    }

    is_running = true;

    /// Split multicast addresses and start receiving in separate threads.
    auto addresses_view = config->get<std::string>("Server.MdAddresses") | std::views::split(',');

    for (auto&& address_part : addresses_view) {
        std::string address(&*address_part.begin(), std::ranges::distance(address_part));

        threads.emplace_back(&CUTMdImpl::odtd_receiver, this, address, config->get<std::string>("Server.InterfaceAddress"));

        logger->info("Subscribed to ODTD multicast address: {} at {}", address, config->get<std::string>("Server.InterfaceAddress"));
    }
}

void trade::broker::CUTMdImpl::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Wrong usage of unsubscribe of CUT: symbols treated as multicast address and must be specified in config");
    }

    is_running = false;

    for (auto&& thread : threads) {
        thread.joinable() ? thread.join() : void();

        /// TODO: Find a way to get multicast address here.
        logger->info("Unsubscribed from ODTD multicast address: {} at {}", "unknown", config->get<std::string>("Server.InterfaceAddress"));
    }
}

void trade::broker::CUTMdImpl::odtd_receiver(const std::string& address, const std::string& interface_addres)
{
    const auto [multicast_ip, multicast_port] = utilities::AddressHelper::extract_address(address);
    utilities::MCClient<char[1024]> client(multicast_ip, multicast_port, interface_addres, true);

    while (is_running) {
        const auto message = client.receive();

        /// Check where the message comes from first.
        const auto exchange_type = CUTCommonData::get_exchange_type(message);

        switch (CUTCommonData::get_tick_type(message, exchange_type)) {
        case types::X_OST_TickType::order: {
            const auto order_tick = CUTCommonData::to_order_tick(message, exchange_type);

            if (order_tick->exchange_time() >= 930000) [[likely]]
                booker.switch_to_continuous_stage();

            booker.add(order_tick);

            break;
        }
        case types::X_OST_TickType::trade: {
            const auto trade_tick = CUTCommonData::to_trade_tick(message, exchange_type);

            booker.trade(trade_tick);

            break;
        }
        default: break;
        }
    }
}
