// Copyright 2020 Fancapital Inc.  All rights reserved.
#include <x/x.h>
#include "./config.h"
#include "yaml-cpp/yaml.h"

namespace co {

    void Config::Init() {
        auto getStr = [&](const YAML::Node& node, const std::string& name) {
            try {
                return node[name] && !node[name].IsNull() ? node[name].as<std::string>() : "";
            } catch (std::exception& e) {
                LOG_ERROR << "load configuration failed: name = " << name << ", error = " << e.what();
                throw std::runtime_error(e.what());
            }
        };
        auto getInt = [&](const YAML::Node& node, const std::string& name, const int64_t& default_value = 0) {
            try {
                return node[name] && !node[name].IsNull() ? node[name].as<int64_t>() : default_value;
            } catch (std::exception& e) {
                LOG_ERROR << "load configuration failed: name = " << name << ", error = " << e.what();
                throw std::runtime_error(e.what());
            }
        };

        auto filename = x::FindFile("feeder.yaml");
        YAML::Node root = YAML::LoadFile(filename);
        opt_ = QOptions::Load(filename);
        auto feeder = root["daapi"];

        daapi_serveraddress_ = getStr(feeder, "daapi_serveraddress");
        daapi_userid_ = getStr(feeder, "daapi_userid");
        std::string _cfg_password = getStr(feeder, "daapi_password");
        daapi_password_ = DecodePassword(_cfg_password);

        daapi_heartbeat_ = getInt(feeder, "daapi_heartbeat");
        daapi_authorcode_ = getStr(feeder, "daapi_authorcode");
        daapi_macdddress_ = getStr(feeder, "daapi_macdddress");
        daapi_computername_ = getStr(feeder, "daapi_computername");
        daapi_softwareversion_ = getStr(feeder, "daapi_softwareversion");
        daapi_softwarename_ = getStr(feeder, "daapi_softwarename");
        daapi_subcode_ = getStr(feeder, "daapi_subcode");
        file_path_ = "../data";

        stringstream ss;
        ss << "+-------------------- configuration begin --------------------+" << endl;
        ss << opt_->ToString() << endl;
        ss << endl;
        ss << "daapi:" << endl
            << "  daapi_serveraddress: " << daapi_serveraddress_ << endl
            << "  daapi_userid: " << daapi_userid_ << endl
            << "  daapi_password: " << string(daapi_password_.size(), '*') << endl
            << "  daapi_heartbeat: " << daapi_heartbeat_ << endl
            << "  daapi_authorcode: " << daapi_authorcode_ << endl
            << "  daapi_macdddress_: " << daapi_macdddress_ << endl
            << "  daapi_computername: " << daapi_computername_ << endl
            << "  daapi_softwareversion: " << daapi_softwareversion_ << endl
            << "  daapi_softwarename: " << daapi_softwarename_ << endl
            << "  daapi_subcode: " << daapi_subcode_ << endl
            << "  file_path: " << file_path_ << endl;
        ss << "+-------------------- configuration end   --------------------+";
        __info << endl << ss.str();
    }
}  // namespace co
