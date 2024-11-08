// Copyright 2020 Fancapital Inc.  All rights reserved.
#pragma once
#include <string>
#include <vector>
#include "feeder/feeder.h"
#include "DAMarketApi.h"
#include "md_buffer.h"

namespace co {
class MDCallBack : public IMarketEvent {
 public:
    MDCallBack(string market);

    virtual ~MDCallBack();

    void Init();

    bool IsLogin();

    virtual void OnFrontConnected();

    virtual void OnFrontDisconnected(int iReason);

    virtual void OnHeartBeatWarning(int iTimeLapse);

    virtual void OnRspUserLogin(CMarketRspInfoField* pRspInfo, int iRequestID, bool bIsLast);

    virtual void OnRspUserLogout(CMarketRspInfoField* pRspInfo, int iRequestID, bool bIsLast);

    virtual void OnRspMarketData(CMarketRspMarketDataField* pRspMarketData, CMarketRspInfoField* pRspInfo,
                                 int iRequestID, bool bIsLast);

 private:
     void SubscFutureMarket();
 private:
    int req_id_;
    bool is_login_;
    CMarketApi* md_api_;
    string market_;
};
}  // namespace co



