// Copyright 2020 Fancapital Inc.  All rights reserved.
#pragma once
#include <boost/atomic.hpp>
#include "config.h"
#include "DAMarketStruct.h"

#define BUFFERLENGTH 8192  // 1048576
#define EPSILON 0.000001

class CommonUtil {
public:
    static int64_t EncodePrice(double price) {
        if (price < 0) {
            return static_cast<int64_t> (price * 10000 - 0.5);
        }
        else {
            return static_cast<int64_t> (price * 10000 + 0.5);
        }
    }
    static double DecodePrice(int64_t price) {
        return static_cast<double> (price) / 10000;
    }
    static bool DB_EQ(double a, double b) {
        return fabs(a - b) < EPSILON;
    }
    static bool DB_GREAT(double a, double b) {
        return (a - b) > EPSILON;
    }
    static bool DB_GREAT_EQ(double a, double b) {
        return (a - b) > EPSILON || DB_EQ(a, b);
    }
    static bool DB_LESS(double a, double b) {
        return (b - a) > EPSILON;
    }
    static bool DB_LESS_EQ(double a, double b) {
        return (b - a) > EPSILON || DB_EQ(a, b);
    }
};

namespace co {
class MDBuffer {
 public:
    MDBuffer();
    ~MDBuffer();
    void Init();
    void Run();
    void AddMdbuffer(CMarketRspMarketDataField* pRspMarketData);
    void ProcessSnapshot(CMarketRspMarketDataField* pRspMarketData);
 private:
    int init_day_;
    mutex mutex_;
    boost::atomic<int> write_index_;
    CMarketRspMarketDataField buffer_[BUFFERLENGTH];
    std::unordered_map<string, std::pair<int64_t, int64_t>> last_tick_;
    int64_t pre_timestamp_ = 0;
};
}  // namespace co
