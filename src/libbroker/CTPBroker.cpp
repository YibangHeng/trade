#include "libbroker/CTPBroker.h"

trade::broker::CTPBrokerImpl::CTPBrokerImpl(const AppBase* parent)
    : m_api(CThostFtdcTraderApi::CreateFtdcTraderApi())
{
    m_api->RegisterSpi(this);
    m_api->RegisterFront(const_cast<char*>(parent->config->get<std::string>("Server.Address").c_str()));
    m_api->SubscribePrivateTopic(THOST_TERT_QUICK);
    m_api->SubscribePublicTopic(THOST_TERT_QUICK);
    m_api->Init();
}

trade::broker::CTPBrokerImpl::~CTPBrokerImpl()
{
    m_api->RegisterSpi(nullptr);
    m_api->Release();
}

trade::broker::CTPBroker::CTPBroker(const std::string& config_path)
    : BrokerProxy("CTPBroker", config_path),
      m_impl(nullptr)
{
}

void trade::broker::CTPBroker::login() noexcept
{
    BrokerProxy::login();

    m_impl = std::make_unique<CTPBrokerImpl>(this);
}
