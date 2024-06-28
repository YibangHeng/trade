#include <utility>

#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/CUTMdImpl.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/ToJSON.hpp"

trade::broker::CUTMdImpl::CUTMdImpl(
    std::shared_ptr<ConfigType> config,
    std::shared_ptr<holder::IHolder> holder,
    std::shared_ptr<reporter::IReporter> reporter
) : AppBase("CUTMdImpl", std::move(config)),
    m_booker_thread_size(AppBase::config->get<size_t>("Performance.BookerConcurrency", std::thread::hardware_concurrency())),
    m_holder(std::move(holder)),
    m_reporter(std::move(reporter))
{
}

void trade::broker::CUTMdImpl::subscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Wrong usage of CUTMdImpl::subscribe: symbols treated as multicast address and must be specified in config");
    }

    m_is_running = true;

    /// Split multicast addresses and start receiving in separate threads.
    auto addresses = config->get<std::string>("Server.MdAddresses");
    std::erase_if(addresses, [](const char c) { return std::isblank(c); });
    auto addresses_view = addresses | std::views::split(',');

    /// Create tick receiver threads.
    for (auto&& address_part : addresses_view) {
        std::string address(&*address_part.begin(), std::ranges::distance(address_part));

        m_tick_receiver_threads.emplace_back(&CUTMdImpl::tick_receiver, this, address, config->get<std::string>("Server.InterfaceAddress"));

        logger->info("Subscribed to ODTD multicast address: {} at {}", address, config->get<std::string>("Server.InterfaceAddress"));
    }

    /// Create booker threads.
    /// Ensure that m_message_buffers does not undergo resizing.
    /// TODO: Is it necessary?
    m_message_buffers.reserve(m_booker_thread_size);

    for (size_t i = 0; i < m_booker_thread_size; i++) {
        auto& message_buffer = m_message_buffers.emplace_back(new MessageBufferType(100000));

        m_booker_threads.emplace_back(&CUTMdImpl::booker, this, std::ref(*message_buffer));

        logger->info("Booker thread {} started", i);
    }
}

void trade::broker::CUTMdImpl::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Wrong usage of CUTMdImpl::unsubscribe: symbols treated as multicast address and must be specified in config");
    }

    m_is_running = false;

    for (auto&& thread : m_tick_receiver_threads) {
        thread.joinable() ? thread.join() : void();

        /// TODO: Find a way to get multicast address here.
        logger->info("Unsubscribed from ODTD multicast address: {} at {}", "unknown", config->get<std::string>("Server.InterfaceAddress"));
    }

    /// Waiting for all booker threads to exit.
    for (auto& booker_thread : m_booker_threads) {
        static size_t booker_thread_id = 0;
        booker_thread.joinable() ? booker_thread.join() : void();

        logger->info("Booker thread {} exited", booker_thread_id++);
    }
}

void trade::broker::CUTMdImpl::tick_receiver(const std::string& address, const std::string& interface_address) const
{
    const auto [multicast_ip, multicast_port] = utilities::AddressHelper::extract_address(address);
    utilities::MCClient<max_udp_size> client(multicast_ip, multicast_port, interface_address);

    /// boost::freelock::queue imposes a constraint that its elements
    /// must have trivial destructors. Consequently, usage of
    /// std::unique_ptr/std::shared_ptr is not viable here.
    /// The message will be deleted in @booker.
    /// TODO: Use memory pool to implement this.
    auto message = new std::vector<u_char>(max_udp_size);

    while (m_is_running) {
        if (!message->empty()) /// For reducing usage of new.
            message = new std::vector<u_char>(max_udp_size);

        client.receive(*message);

        /// Non-blocking receiver may return empty data.
        if (message->empty())
            continue;

        const int64_t symbol = CUTCommonData::get_symbol_from_message(*message);

        while (!m_message_buffers[symbol % m_message_buffers.size()]->push(message)) {
            if (!m_is_running) [[unlikely]]
                break;

            logger->warn("Message buffer is full, which may cause data dropping");
        }
    }
}

void trade::broker::CUTMdImpl::booker(MessageBufferType& message_buffer)
{
    booker::Booker booker({}, m_reporter, config->get<bool>("Performance.EnableVerification", false)); /// TODO: Initialize tradable symbols here.

    while (m_is_running) {
        std::vector<u_char>* message;

        if (!message_buffer.pop(message))
            continue;

        booker::OrderTickPtr order_tick;
        booker::TradeTickPtr trade_tick;
        booker::L2TickPtr l2_tick;

        switch (message->size()) {
        case sizeof(SSEHpfTick): order_tick = CUTCommonData::to_order_tick<SSEHpfTick>(*message); break;
        case sizeof(SSEHpfL2Snap): l2_tick = CUTCommonData::to_l2_tick<SSEHpfL2Snap>(*message); break;
        case sizeof(SZSEHpfOrderTick): order_tick = CUTCommonData::to_order_tick<SZSEHpfOrderTick>(*message); break;
        case sizeof(SZSEHpfTradeTick): trade_tick = CUTCommonData::to_trade_tick<SZSEHpfTradeTick>(*message); break;
        case sizeof(SZSEHpfL2Snap): l2_tick = CUTCommonData::to_l2_tick<SZSEHpfL2Snap>(*message); break;
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

        /// boost::freelock::queue imposes a constraint that its elements
        /// must have trivial destructors. Consequently, usage of
        /// std::unique_ptr/std::shared_ptr is not viable here.
        /// We need delete message manually.
        delete message;
    }
}
