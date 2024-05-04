#pragma once

#include <memory>

#include "AppBase.hpp"
#include "BrokerProxy.h"
#include "third/ctp/ThostFtdcTraderApi.h"

namespace trade::broker
{

class CTPBrokerImpl final: private CThostFtdcTraderSpi
{
public:
    explicit CTPBrokerImpl(const AppBase* parent);
    virtual ~CTPBrokerImpl();

private:
    CThostFtdcTraderApi* m_api;
};

class PUBLIC_API CTPBroker final: public BrokerProxy
{
public:
    explicit CTPBroker(const std::string& config_path);
    ~CTPBroker() override = default;

public:
    void login() noexcept override;

private:
    std::unique_ptr<CTPBrokerImpl> m_impl;
};

} // namespace trade::broker
