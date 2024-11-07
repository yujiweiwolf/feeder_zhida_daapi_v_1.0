#include "config.h"
#include "daapi_server.h"
#include <boost/algorithm/string.hpp>

namespace co {
    DaapiServer::DaapiServer() {
    }

    DaapiServer::~DaapiServer() {
        for(auto& it : call_back_) {
            if (it) {
                delete it;
                it = nullptr;
            }
        }
    }

    void DaapiServer::Run() {
        auto opt = Singleton<Config>::GetInstance()->opt();
        feeder_server_ = std::make_shared<FeedServer>();
        feeder_server_->Init(opt, opt->sub_date());
        feeder_server_->Start();
        Singleton<MDBuffer>::Instance(feeder_server_);
        Singleton<MDBuffer>::GetInstance()->Init();

        string daapi_subcode = Singleton<Config>::GetInstance()->daapi_subcode();
        vector<string> vec_info;
        boost::split(vec_info, daapi_subcode, boost::is_any_of(";"), boost::token_compress_on);
        for (auto& it : vec_info) {
            MDCallBack* pCallBack = new MDCallBack(it);
            pCallBack->Init();
            call_back_.push_back(pCallBack);
            x::Sleep(100);
        }
        feeder_server_->Wait();
        feeder_server_->Stop();
    }
}  // namespace co

