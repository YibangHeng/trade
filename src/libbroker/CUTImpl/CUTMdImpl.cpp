#include <cmath>
#include <pcap.h>
#include <utility>

#include "libbroker/CUTImpl/CUTCommonData.h"
#include "libbroker/CUTImpl/CUTMdImpl.h"
#include "libbroker/CUTImpl/RawStructure.h"
#include "utilities/AddressHelper.hpp"
#include "utilities/NetworkHelper.hpp"
#include "utilities/ToJSON.hpp"
#include "utilities/UdpPayLoadGetter.hpp"

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
        throw std::runtime_error("Subscribing for specific symbols is not supported for CUTMdImpl yet");
    }

    m_is_running = true;

    /// Create booker threads.
    /// Ensure that m_message_buffers does not undergo resizing.
    /// TODO: Is it necessary?
    m_message_buffers.reserve(m_booker_thread_size);

    for (size_t i = 0; i < m_booker_thread_size; i++) {
        auto& message_buffer = m_message_buffers.emplace_back(new MessageBufferType);

        m_booker_threads.emplace_back(&CUTMdImpl::booker, this, std::ref(*message_buffer));

        logger->info("Booker thread {} started", i);
    }

    /// Create tick receiver threads.
    m_tick_receiver_thread = std::thread(&CUTMdImpl::tick_receiver, this);
}

void trade::broker::CUTMdImpl::unsubscribe(const std::unordered_set<std::string>& symbols)
{
    if (!symbols.empty()) {
        throw std::runtime_error("Unsubscribing for specific symbols is not supported for CUTMdImpl yet");
    }

    m_is_running = false;

    m_tick_receiver_thread.joinable() ? m_tick_receiver_thread.join() : void();

    logger->info("Tick receiver thread exited");

    /// Waiting for all booker threads to exit.
    for (auto& booker_thread : m_booker_threads) {
        static size_t booker_thread_id = 0;
        booker_thread.joinable() ? booker_thread.join() : void();

        logger->info("Booker thread {} exited", booker_thread_id++);
    }
}

void trade::broker::CUTMdImpl::tick_receiver() const
{
    char errbuf[PCAP_ERRBUF_SIZE];
    const auto handle = pcap_fopen_offline(stdin, errbuf);

    if (handle == nullptr) {
        logger->error("Failed to read PCAP stream: {}", errbuf);
        return;
    }

    pcap_pkthdr header {};
    size_t udp_payload_length;

    while (m_is_running) {
        const u_char* packet = pcap_next(handle, &header);

        if (packet == nullptr)
            continue;

        /// boost::freelock::queue imposes a constraint that its elements
        /// must have trivial destructors. Consequently, usage of
        /// std::unique_ptr/std::shared_ptr is not viable here.
        /// The message will be deleted in @booker.
        /// TODO: Use memory pool to implement this.
        const auto message = new std::vector<u_char>(max_udp_size);

        const auto payload = utilities::UdpPayLoadGetter()(packet, header.caplen, udp_payload_length);

        /// Copy udp payload to message.
        message->resize(udp_payload_length);
        std::copy_n(payload, udp_payload_length, message->begin());

        const int64_t symbol = CUTCommonData::get_symbol_from_message(*message);

        /// For gradual sleep time.
        static size_t full_counter = 0;

        while (!m_message_buffers[symbol % m_message_buffers.size()]->push(message)) {
            if (!m_is_running) [[unlikely]]
                break;

            logger->warn("Message buffer is full, which may cause data dropping. Sleeping {}ms", std::pow(2, full_counter));
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int64_t>(std::pow(2, full_counter++))));
        }

        full_counter = 0;

        /// Simulate l2 snap tick interval.
        std::this_thread::sleep_for(std::chrono::milliseconds(config->get<int>("Mock.L2SnapTickInterval", 3000)));
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
        case sizeof(SSEHpfL2Snap): l2_tick = CUTCommonData::to_exchange_l2_tick<SSEHpfL2Snap>(*message); break;
        case sizeof(SZSEHpfOrderTick): order_tick = CUTCommonData::to_order_tick<SZSEHpfOrderTick>(*message); break;
        case sizeof(SZSEHpfTradeTick): trade_tick = CUTCommonData::to_trade_tick<SZSEHpfTradeTick>(*message); break;
        case sizeof(SZSEHpfL2Snap): l2_tick = CUTCommonData::to_exchange_l2_tick<SZSEHpfL2Snap>(*message); break;
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

            m_reporter->exchange_l2_tick_arrived(l2_tick);
        }

        /// boost::freelock::queue imposes a constraint that its elements
        /// must have trivial destructors. Consequently, usage of
        /// std::unique_ptr/std::shared_ptr is not viable here.
        /// We need delete message manually.
        delete message;
    }
}
