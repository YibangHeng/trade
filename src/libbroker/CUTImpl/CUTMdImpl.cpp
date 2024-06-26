#include <regex>

#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/CUTMdImpl.h"
#include "utilities/NetworkHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::broker::CUTMdImpl::CUTMdImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter
) : AppBase("CUTMdImpl", std::move(config)),
    thread(nullptr),
    m_holder(std::move(holder)),
    m_reporter(std::move(reporter))
{
}

void trade::broker::CUTMdImpl::subscribe(const std::unordered_set<std::string>& symbols)
{
    if (symbols.size() != 1) {
        logger->error("Wrong usage of subscribe of CUT: symbols treated as multicast address and must be exactly one");
        return;
    }

    is_running = true;

    /// Start receiving in a separate thread.
    thread = new std::thread([this, &symbols] {
        const std::string address = *symbols.begin(); /// Make a copy to avoid dangling reference.
        odtd_receiver(address);
    });

    logger->info("Subscribed to ODTD multicast address: {}", *symbols.begin());
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

void trade::broker::CUTMdImpl::odtd_receiver(const std::string& address) const
{
    const auto [multicast_address, multicast_port] = extract_address(address);
    utilities::MCClient<char[1024]> client(multicast_address, multicast_port);

    while (is_running) {
        const auto message      = client.receive();

        const auto message_type = CUTCommonData::get_datagram_type(message);

        switch (message_type) {
        case types::X_OST_SZSEDatagramType::order: {
            const auto order_tick = CUTCommonData::to_order_tick(message);

            logger->debug("Received order tick: {}", utilities::ToJSON()(order_tick));

            break;
        }
        case types::X_OST_SZSEDatagramType::trade: {
            const auto trade_tick = CUTCommonData::to_trade_tick(message);

            logger->debug("Received trade tick: {}", utilities::ToJSON()(trade_tick));

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
