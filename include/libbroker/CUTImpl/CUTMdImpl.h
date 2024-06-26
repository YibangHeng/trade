#pragma once

#include "AppBase.hpp"
#include "libholder/IHolder.h"
#include "libreporter/IReporter.hpp"
#include "third/ctp/ThostFtdcMdApi.h"
#include "utilities/LoginSyncer.hpp"

namespace trade::broker
{

class CUTMdImpl final: private AppBase<int>, private CThostFtdcMdSpi
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
    void odtd_receiver(const std::string& address) const;

private:
    static std::tuple<std::string, uint16_t> extract_address(const std::string& address);

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
