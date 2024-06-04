#include <regex>

#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/CUTMdImpl.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "utilities/NetworkHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::broker::CUTMdImpl::CUTMdImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter
) : AppBase("CUTMdImpl", std::move(config)),
    booker({}, reporter), /// TODO: Initialize tradable symbols here.
    thread(nullptr),
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

    /// Start receiving in a separate thread.
    thread = new std::thread([this] {
        odtd_receiver(config->get<std::string>("Server.MdAddress"));
    });

    logger->info("Subscribed to ODTD multicast address: {}", config->get<std::string>("Server.MdAddress"));
}

void trade::broker::CUTMdImpl::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    if (symbols.size() != 1) {
        logger->error("Wrong usage of unsubscribe of CUT: symbols treated as multicast address and must be exactly one");
        return;
    }

    is_running = false;

    if (thread->joinable())
        thread->join();

    logger->info("Unsubscribed from ODTD multicast address: {}", *symbols.begin());

    delete thread;
}

void trade::broker::CUTMdImpl::odtd_receiver(const std::string& address)
{
    const auto [multicast_address, multicast_port] = extract_address(address);
    utilities::MCClient<char[1024]> client(multicast_address, multicast_port);

    while (is_running) {
        const auto message = client.receive();

        /// TODO: Is it OK to check message type by message size?
        types::ExchangeType exchange_type;
        if (message.size() == sizeof(SSEHpfOrderTick) || message.size() == sizeof(SSEHpfTradeTick))
            exchange_type = types::ExchangeType::sse;
        else if (message.size() == sizeof(SZSEHpfOrderTick) || message.size() == sizeof(SZSEHpfTradeTick))
            exchange_type = types::ExchangeType::szse;
        else {
            logger->error("Invalid message size: received {} bytes, expected 64, 72 or 96", message.size());
            continue;
        }

        switch (CUTCommonData::get_datagram_type(message, exchange_type)) {
        case types::X_OST_DatagramType::order: {
            const auto order_tick = CUTCommonData::to_order_tick(message, exchange_type);

            logger->debug("Received order tick: {}", utilities::ToJSON()(*order_tick));

            if (order_tick->exchange_time() >= 930000) [[likely]]
                booker.switch_to_continuous_stage();

            booker.add(order_tick);

            break;
        }
        case types::X_OST_DatagramType::trade: {
            const auto trade_tick = CUTCommonData::to_trade_tick(message, exchange_type);

            logger->debug("Received trade tick: {}", utilities::ToJSON()(*trade_tick));

            booker.trade(trade_tick);

            break;
        }
        default: break;
        }
    }
}

/// Extract multicast address and port from address string.
std::tuple<std::string, uint16_t> trade::broker::CUTMdImpl::extract_address(const std::string& address)
{
    const std::regex re(R"(^(\d{1,3}(?:\.\d{1,3}){3}):(\d+)$)");
    std::smatch address_match;

    std::regex_search(address, address_match, re);

    if (address_match.size() == 3) {
        return std::make_tuple(std::string(address_match[1]), static_cast<uint16_t>(std::stoi(address_match[2])));
    }

    return std::make_tuple(std::string {}, static_cast<uint16_t>(0));
}
