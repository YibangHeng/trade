#pragma once

#include <boost/lockfree/spsc_queue.hpp>
#include <pcap/pcap.h>

#include "AppBase.hpp"
#include "libbooker/Booker.h"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "third/ctp/ThostFtdcMdApi.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class CUTMdImpl final: private AppBase<int>,
                       private CThostFtdcMdSpi
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

private:
    using MessageBufferType = boost::lockfree::spsc_queue<std::vector<u_char>*, boost::lockfree::capacity<100000000>>;

    pcap_t* init_pcap_handle(
        const std::string& interface,
        const std::string& filter
    ) const;
    pcap_dumper_t* init_pcap_dumper(
        pcap_t* handle,
        std::string dump_file
    ) const;
    void tick_receiver();
    void booker(MessageBufferType& message_buffer);

private:
    std::atomic<bool> m_is_running;
    std::thread m_tick_receiver_thread;
    size_t m_booker_thread_size;
    std::vector<std::thread> m_booker_threads;
    std::vector<std::unique_ptr<MessageBufferType>> m_message_buffers;

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
