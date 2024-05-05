#pragma once

#include <memory>

#include "AppBase.hpp"
#include "BrokerProxy.hpp"
#include "third/ctp/ThostFtdcTraderApi.h"

namespace trade::broker
{

class CTPBroker;

class CTPBrokerImpl final: private AppBase<int>, private CThostFtdcTraderSpi
{
public:
    explicit CTPBrokerImpl(CTPBroker* parent);
    ~CTPBrokerImpl() override;

private:
    void OnFrontConnected() override;
    void OnRspUserLogin(
        CThostFtdcRspUserLoginField* pRspUserLogin,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;
    void OnRspUserLogout(
        CThostFtdcUserLogoutField* pUserLogout,
        CThostFtdcRspInfoField* pRspInfo,
        int nRequestID, bool bIsLast
    ) override;

private:
    CThostFtdcTraderApi* m_api;

private:
    CTPBroker* m_parent;
};

class PUBLIC_API CTPBroker final: public BrokerProxy<int>
{
public:
    explicit CTPBroker(const std::string& config_path);
    ~CTPBroker() override = default;

public:
    void login() noexcept override;
    void logout() noexcept override;

private:
    std::unique_ptr<CTPBrokerImpl> m_impl;
};

} // namespace trade::broker
