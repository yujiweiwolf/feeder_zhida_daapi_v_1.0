// Copyright 2020 Fancapital Inc.  All rights reserved.
#pragma once
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include "feeder/feeder.h"

template <typename T>
class Singleton {
 public:
    template<typename... Args>
    static T* Instance(Args&&... args) {
        if (m_pInstance == nullptr)
            m_pInstance = new T(std::forward<Args>(args)...);

        return m_pInstance;
    }

    static T* GetInstance() {
        if (m_pInstance == nullptr)
            throw std::logic_error("please initialize the instance first");

        return m_pInstance;
    }

    static void DestroyInstance() {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

 private:
    Singleton(void) {}
    virtual ~Singleton(void) {}
    Singleton(const Singleton&) {}
    Singleton& operator=(const Singleton&) {}

 private:
    static T* m_pInstance;
};

template <class T> T* Singleton<T>::m_pInstance = nullptr;


namespace co {
class Config {
 public:
    inline string daapi_serveraddress() {
        return daapi_serveraddress_;
    }
    inline string daapi_userid() {
        return daapi_userid_;
    }
    inline string daapi_password() {
        return daapi_password_;
    }
    inline int daapi_heartbeat() {
        return daapi_heartbeat_;
    }
    inline string daapi_authorcode() {
        return daapi_authorcode_;
    }
    inline string daapi_macdddress() {
        return daapi_macdddress_;
    }
    inline string daapi_computername() {
        return daapi_computername_;
    }
    inline string daapi_softwareversion() {
        return daapi_softwareversion_;
    }
    inline string daapi_softwarename() {
        return daapi_softwarename_;
    }
    inline string daapi_subcode() {
        return daapi_subcode_;
    }
    inline string file_path() {
        return file_path_;
    }
    inline std::shared_ptr<FeedOptions> opt() {
        return opt_;
    }
    void Init();

 private:
    std::shared_ptr<FeedOptions> opt_;
    string daapi_serveraddress_;
    string daapi_userid_;
    string daapi_password_;
    int daapi_heartbeat_;
    string daapi_authorcode_;
    string daapi_macdddress_;
    string daapi_computername_;
    string daapi_softwareversion_;
    string daapi_softwarename_;
    string daapi_subcode_;
    string file_path_;
};
}  // namespace co
