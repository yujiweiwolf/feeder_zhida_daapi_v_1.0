// Copyright 2020 Fancapital Inc.  All rights reserved.
#include <memory>
#include "./config.h"
#include "./md_callback.h"

namespace co {
    MDCallBack::MDCallBack(string market): IMarketEvent(), market_(market){
        req_id_ = 0;
        is_login_ = false;
    }

    MDCallBack::~MDCallBack() {
        if (md_api_) {
            if (is_login_) {
                is_login_ = false;
                x::Sleep(1000);
            }

            md_api_->Release();
            md_api_ = NULL;
        }
    }

    void MDCallBack::Init() {
        string file = market_ + "_Market.log";
        md_api_ = CMarketApi::CreateMarketApi(true, file.c_str());
        if (md_api_) {
            LOG_INFO << market_ + ", API Version: " << md_api_->GetVersion();
            md_api_->RegisterSpi(this);
            md_api_->RegisterNameServer(Singleton<Config>::GetInstance()->daapi_serveraddress().c_str());
            md_api_->SetHeartBeatTimeout(Singleton<Config>::GetInstance()->daapi_heartbeat());
            md_api_->Init();
        }
    }

    bool MDCallBack::IsLogin() {
        return is_login_;
    }

    void MDCallBack::OnFrontConnected() {
        LOG_INFO << market_ + ", MdApi OnFrontConnected";
        CMarketReqUserLoginField field;
        memset(&field, 0, sizeof(field));
        string _daapi_userid = Singleton<Config>::GetInstance()->daapi_userid();
        strncpy(field.UserId, _daapi_userid.c_str(), _daapi_userid.length());

        string _daapi_password = Singleton<Config>::GetInstance()->daapi_password();
        strncpy(field.UserPwd, _daapi_password.c_str(), _daapi_password.length());

        string _daapi_authorcode = Singleton<Config>::GetInstance()->daapi_authorcode();
        strncpy(field.AuthorCode, _daapi_authorcode.c_str(), _daapi_authorcode.length());

        string _daapi_macdddress = Singleton<Config>::GetInstance()->daapi_macdddress();
        strncpy(field.MacAddress, _daapi_macdddress.c_str(), _daapi_macdddress.length());

        string _daapi_computername = Singleton<Config>::GetInstance()->daapi_computername();
        strncpy(field.ComputerName, _daapi_computername.c_str(), _daapi_computername.length());

        string _daapi_softwareversion = Singleton<Config>::GetInstance()->daapi_softwareversion();
        strncpy(field.SoftwareName, _daapi_softwareversion.c_str(), _daapi_softwareversion.length());

        string _daapi_softwarename_ = Singleton<Config>::GetInstance()->daapi_softwarename();
        strncpy(field.SoftwareVersion, _daapi_softwarename_.c_str(), _daapi_softwarename_.length());

        md_api_->ReqUserLogin(&field, req_id_++);
    }

    void MDCallBack::SubscFutureMarket() {
        CMarketReqMarketDataField field;
        memset(&field, 0, sizeof(field));
        field.SubscMode = DAF_SUB_Append;
        field.MarketType = DAF_TYPE_Future;
        //// �ֱ���������������Ʒ��
        //field.MarketCount = 2;
        //string temp = "SGXQ,CN*";
        //strncpy(field.MarketTrcode[0], temp.c_str(), temp.length());
        //temp = "HKEX,HSI*";
        //strncpy(field.MarketTrcode[1], temp.c_str(), temp.length());

        string _sub_context = market_ + ",*";
        field.MarketCount = 1;
        //strcpy(field.MarketTrcode[0], "HKEX,*");
        strcpy(field.MarketTrcode[0], _sub_context.c_str());
        int ret = md_api_->ReqMarketData(&field, ++req_id_);
        if (ret > 0) {
            LOG_INFO << market_ + ", SubscFutureMarket succeed" << endl;
        } else {
            LOG_ERROR << market_ + ", SubscFutureMarket failded" << endl;
        }
    }

    void MDCallBack::OnFrontDisconnected(int nReason) {
        is_login_ = false;
        LOG_ERROR << market_ + ", OnFrontDisconnected nReason = " << nReason;
    }

    void MDCallBack::OnHeartBeatWarning(int iTimeLapse) {
        // LOG_INFO << " OnHeartBeatWarning " << iTimeLapse;
    }

    void MDCallBack::OnRspUserLogin(CMarketRspInfoField* pRspInfo, int iRequestID, bool bIsLast) {
        if (pRspInfo->ErrorID == 0) {
            is_login_ = true;
            LOG_INFO << market_ + ", OnRspUserLogin Succeed";
            SubscFutureMarket();
        } else {
            LOG_ERROR << market_ + ",OnRspUserLogin ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg;
        }
    }

    void MDCallBack::OnRspUserLogout(CMarketRspInfoField* pRspInfo, int iRequestID, bool bIsLast) {
        if (pRspInfo->ErrorID == 0) {
            is_login_ = false;
            LOG_INFO << market_ + ", OnRspUserLogout";
        } else {
            LOG_ERROR << market_ + ", OnRspUserLogout ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg;
        }
    }

    void MDCallBack::OnRspMarketData(CMarketRspMarketDataField* pRspMarketData, CMarketRspInfoField* pRspInfo,
                                     int iRequestID, bool bIsLast) {
        if (pRspInfo->ErrorID) {
            LOG_ERROR << "OnRspUserLogin ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg;
        } else if (pRspMarketData) {
            Singleton<MDBuffer>::GetInstance()->AddMdbuffer(pRspMarketData);
        }
    }
}  // namespace co
