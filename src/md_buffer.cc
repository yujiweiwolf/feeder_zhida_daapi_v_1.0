#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <x/x.h>
#include "coral/coral.h"
#include <filesystem>
#include "md_buffer.h"
#include "feeder/feeder.h"

namespace fs = std::filesystem;

#define MINPRICE 0.000001
#define MAXPIRCE 1000000.0

namespace co {
    MDBuffer::MDBuffer() {
        for (int _i = 0; _i < BUFFERLENGTH; _i++) {
            memset(buffer_ + _i, 0, sizeof(CMarketRspMarketDataField));
        }
        init_day_ = x::RawDate();
        pre_timestamp_ = x::RawDateTime();
    }

    MDBuffer::~MDBuffer() {
    }

    void MDBuffer::Init() {
        write_index_.store(0);;
        std::shared_ptr<std::thread> thread = std::make_shared<std::thread>(std::bind(&MDBuffer::Run, this));
        thread->detach();
    }

    void MDBuffer::Run() {
        int _read_index = 0;
        while (true) {
            while (_read_index < write_index_.load()) {
                int mod_index = _read_index & (BUFFERLENGTH - 1);
                CMarketRspMarketDataField* pRspMarketData = buffer_ + mod_index;
                if (mod_index == 0) {
                    __info << " read_index: " << _read_index
                        << ", mod : " << mod_index
                        << ", " << pRspMarketData->TreatyCode
                        << ", " << pRspMarketData->ExchangeCode
                        << ", " << pRspMarketData->BuyPrice
                        << ", " << pRspMarketData->BuyNumber
                        << ", " << pRspMarketData->SalePrice
                        << ", " << pRspMarketData->SaleNumber
                        << ", TradeDay: " << pRspMarketData->TradeDay
                        << ", Time: " << pRspMarketData->Time
                        << ", DataTimestamp: " << pRspMarketData->DataTimestamp;
                }
                if (strlen(pRspMarketData->TreatyCode) > 5 && strlen(pRspMarketData->Time) > 10) {
                    ProcessSnapshot(pRspMarketData);
                }
                ++_read_index;
            }
        }
    }

    // 多线程调用此函数
    void MDBuffer::AddMdbuffer(CMarketRspMarketDataField* pRspMarketData) {
        /*if (strcmp(pRspMarketData->TreatyCode, "CN2110") == 0) {
            pRspMarketData->Time[9] = '5';
            __info << ", " << pRspMarketData->TreatyCode
                << ", " << pRspMarketData->ExchangeCode
                << ", Open: " << pRspMarketData->Open
                << ", CurrPrice: " << pRspMarketData->CurrPrice
                << ", CurrNumber: " << pRspMarketData->CurrNumber
                << ", BuyPrice: " << pRspMarketData->BuyPrice
                << ", BuyNumber: " << pRspMarketData->BuyNumber
                << ", SalePrice: " << pRspMarketData->SalePrice
                << ", SaleNumber: " << pRspMarketData->SaleNumber
                << ", TradeDay: " << pRspMarketData->TradeDay
                << ", Time: " << pRspMarketData->Time
                << ", DataTimestamp: " << pRspMarketData->DataTimestamp;
        } else {
            return;
        }*/

        std::unique_lock<std::mutex> lock(mutex_);
        int now_index = write_index_.load();
        int mod_index = now_index &(BUFFERLENGTH - 1);
        if (mod_index == 0) {
            __info << "write_index: " << write_index_.load()
                << ", mod : " << mod_index
                << ", " << pRspMarketData->TreatyCode
                << ", " << pRspMarketData->ExchangeCode
                << ", " << pRspMarketData->BuyPrice
                << ", " << pRspMarketData->BuyNumber
                << ", " << pRspMarketData->SalePrice
                << ", " << pRspMarketData->SaleNumber
                << ", TradeDay: " << pRspMarketData->TradeDay
                << ", Time: " << pRspMarketData->Time
                << ", DataTimestamp: " << pRspMarketData->DataTimestamp;
        }
        CMarketRspMarketDataField* now_data = buffer_ + mod_index;
        memcpy(buffer_ + mod_index, pRspMarketData, sizeof(CMarketRspMarketDataField));
        write_index_.add(1);
    }

    void MDBuffer::ProcessSnapshot(CMarketRspMarketDataField* pRspMarketData) {
        int64_t market = 0;
        string _exchange = x::Trim(pRspMarketData->ExchangeCode);
        if (_exchange == "SGXQ") {
            market = kMarketSGX;
        } else if (_exchange == "HKEX") {
            market = kMarketHK;
        } else if (_exchange == "CME") {
            market = kMarketCME;
        } else if (_exchange == "CME_CBT") {
            market = kMarketCMECBT;
        } else if (_exchange == "NYBOT") {
            market = kMarketNYBOT;
        } else if (_exchange == "LME") {
            market = kMarketLME;
        } else if (_exchange == "ICE") {
            market = kMarketICE;
        } else if (_exchange == "XEurex") {
            market = kMarketXEUREX;
        } else if (_exchange == "TOCOM") {
            market = kMarketTOCOM;
        } else if (_exchange == "KRX") {
            market = kMarketKRX;
        } else {
            __warn << "not valid code: " << pRspMarketData->TreatyCode << ", exchange: " << _exchange;
            return;
        }

        string _std_code = x::Trim(pRspMarketData->TreatyCode) + Market2Suffix(market);
        QContextPtr _ctx = QServer::Instance()->GetContext(_std_code);
        if (!_ctx) {
            _ctx = std::make_shared< QContext >(_std_code);
            co::fbs::QTickT& _tmp_qtick = _ctx->tick();
            _tmp_qtick.dtype = co::kDTypeFuture;
            QServer::Instance()->SetContext(_std_code, _ctx);
         }

        co::fbs::QTickT& _tmp_qtick = _ctx->PrepareQTick();
        string _timestamp;
        for (int Index = 0; Index < static_cast<int>(strlen(pRspMarketData->Time)); ++Index) {
            if (pRspMarketData->Time[Index] >= '0' && pRspMarketData->Time[Index] <= '9') {
                _timestamp += pRspMarketData->Time[Index];
            }
        }
        int tick_day = atoll(_timestamp.c_str()) / 1000000LL;

        if (tick_day < init_day_) {
            __error << "not valid time day: " << tick_day;
            return;
        }
        _tmp_qtick.code = _std_code;
        _tmp_qtick.timestamp = atoll(_timestamp.c_str()) * 1000LL;
        // [future][05:44:58, 09:30:28] + 6687 /   787980, delay: 196887ms,   6C2203.CME=269ms,  NIY2110.CME=13524434ms
        // 比之前的时间戳慢了1小时
        int64_t diff = pre_timestamp_ - _tmp_qtick.timestamp;
        if (diff > 5900000) {
            __error << "not valid time stamp, pre: " << pre_timestamp_ << ", " << pRspMarketData->TreatyCode
                << ", " << pRspMarketData->ExchangeCode << ", Time: " << pRspMarketData->Time;
            return;
        }

        _tmp_qtick.new_price = atof(pRspMarketData->CurrPrice);
        _tmp_qtick.new_volume = atoll(pRspMarketData->CurrNumber);
        _tmp_qtick.sum_volume = atoll(pRspMarketData->FilledNum);
        auto it = last_tick_.find(_std_code);
        if (it == last_tick_.end()) {
            last_tick_.insert(std::make_pair(_std_code, std::make_pair(_tmp_qtick.timestamp, _tmp_qtick.sum_volume)));
        } else {
            // 时间戳变小
            if (it->second.first > _tmp_qtick.timestamp) {
                /*__error << pRspMarketData->TreatyCode << ", " << pRspMarketData->ExchangeCode
                    << ", not valid now timestamp: " << _tmp_qtick.timestamp << ", pre timestamp: " << it->second.first;*/
                return;
            }

            //// 成交数量变小
            if (it->second.second > _tmp_qtick.sum_volume && _tmp_qtick.sum_volume > 0) {
                /*__error << pRspMarketData->TreatyCode << ", " << pRspMarketData->ExchangeCode
                    << ", not valid now sum_volume: " << _tmp_qtick.sum_volume << ", pre sum_volume: " << it->second.second;*/
                return;
            }
            it->second.first = _tmp_qtick.timestamp;
            it->second.second = _tmp_qtick.sum_volume;
        }

        double _temp_price = atof(pRspMarketData->PreSettlementPrice);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.pre_close = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->Open);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.open = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->High);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.high = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->Low);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.low = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->Close);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.close = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->LimitUpPrice);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.upper_limit = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->LimitDownPrice);
        if (_temp_price < MAXPIRCE) {
            _tmp_qtick.lower_limit = _temp_price;
        }

        double price = 0.0;
        int64_t volume = 0;
        price = atof(pRspMarketData->BuyPrice);
        volume = atoll(pRspMarketData->BuyNumber);
        if (price > MINPRICE && volume > 0) {
            _tmp_qtick.bp.push_back(price);
            _tmp_qtick.bv.push_back(volume);
            price = atof(pRspMarketData->BuyPrice2);
            volume = atoll(pRspMarketData->BuyNumber2);
            if (price > MINPRICE && volume > 0) {
                _tmp_qtick.bp.push_back(price);
                _tmp_qtick.bv.push_back(volume);
                price = atof(pRspMarketData->BuyPrice3);
                volume = atoll(pRspMarketData->BuyNumber3);
                if (price > MINPRICE && volume > 0) {
                    _tmp_qtick.bp.push_back(price);
                    _tmp_qtick.bv.push_back(volume);
                    price = atof(pRspMarketData->BuyPrice4);
                    volume = atoll(pRspMarketData->BuyNumber4);
                    if (price > MINPRICE && volume > 0) {
                        _tmp_qtick.bp.push_back(price);
                        _tmp_qtick.bv.push_back(volume);
                        price = atof(pRspMarketData->BuyPrice5);
                        volume = atoll(pRspMarketData->BuyNumber5);
                        if (price > MINPRICE && volume > 0) {
                            _tmp_qtick.bp.push_back(price);
                            _tmp_qtick.bv.push_back(volume);
                            price = atof(pRspMarketData->BuyPrice6);
                            volume = atoll(pRspMarketData->BuyNumber6);
                            if (price > MINPRICE && volume > 0) {
                                _tmp_qtick.bp.push_back(price);
                                _tmp_qtick.bv.push_back(volume);
                                price = atof(pRspMarketData->BuyPrice7);
                                volume = atoll(pRspMarketData->BuyNumber7);
                                if (price > MINPRICE && volume > 0) {
                                    _tmp_qtick.bp.push_back(price);
                                    _tmp_qtick.bv.push_back(volume);
                                    price = atof(pRspMarketData->BuyPrice8);
                                    volume = atoll(pRspMarketData->BuyNumber8);
                                    if (price > MINPRICE && volume > 0) {
                                        _tmp_qtick.bp.push_back(price);
                                        _tmp_qtick.bv.push_back(volume);
                                        price = atof(pRspMarketData->BuyPrice9);
                                        volume = atoll(pRspMarketData->BuyNumber9);
                                        if (price > MINPRICE && volume > 0) {
                                            _tmp_qtick.bp.push_back(price);
                                            _tmp_qtick.bv.push_back(volume);
                                            price = atof(pRspMarketData->BuyPrice10);
                                            volume = atoll(pRspMarketData->BuyNumber10);
                                            if (price > MINPRICE && volume > 0) {
                                                _tmp_qtick.bp.push_back(price);
                                                _tmp_qtick.bv.push_back(volume);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        // 检查bp的有效性
        /*_tmp_qtick.bp = { 9, 8, 7, 6, 5, 6};
        _tmp_qtick.bv = { 1, 1, 1, 1, 1, 1 };*/
        if (_tmp_qtick.bp.size() > 1) {
            double pre_bp = _tmp_qtick.bp[0];
            size_t index = 1;
            for(; index < _tmp_qtick.bp.size(); index ++) {
                if (_tmp_qtick.bp[index] > pre_bp) {
                    __error << "bid price change big, now: " << _tmp_qtick.bp[index]
                        << ", pre: " << pre_bp;
                    break;
                } else {
                    pre_bp = _tmp_qtick.bp[index];
                }
            }
            if (index < _tmp_qtick.bp.size()) {
                vector<double> bp;
                bp.swap(_tmp_qtick.bp);
                _tmp_qtick.bp.assign(bp.begin(), bp.begin() + index);

                vector<int64_t> bv;
                bv.swap(_tmp_qtick.bv);
               _tmp_qtick.bv.assign(bv.begin(), bv.begin() + index);
            }
        }

        price = atof(pRspMarketData->SalePrice);
        volume = atoll(pRspMarketData->SaleNumber);
        if (price > MINPRICE && volume > 0) {
            _tmp_qtick.ap.push_back(price);
            _tmp_qtick.av.push_back(volume);
            price = atof(pRspMarketData->SalePrice2);
            volume = atoll(pRspMarketData->SaleNumber2);
            if (price > MINPRICE && volume > 0) {
                _tmp_qtick.ap.push_back(price);
                _tmp_qtick.av.push_back(volume);
                price = atof(pRspMarketData->SalePrice3);
                volume = atoll(pRspMarketData->SaleNumber3);
                if (price > MINPRICE && volume > 0) {
                    _tmp_qtick.ap.push_back(price);
                    _tmp_qtick.av.push_back(volume);
                    price = atof(pRspMarketData->SalePrice4);
                    volume = atoll(pRspMarketData->SaleNumber4);
                    if (price > MINPRICE && volume > 0) {
                        _tmp_qtick.ap.push_back(price);
                        _tmp_qtick.av.push_back(volume);
                        price = atof(pRspMarketData->SalePrice5);
                        volume = atoll(pRspMarketData->SaleNumber5);
                        if (price > MINPRICE && volume > 0) {
                            _tmp_qtick.ap.push_back(price);
                            _tmp_qtick.av.push_back(volume);
                            price = atof(pRspMarketData->SalePrice6);
                            volume = atoll(pRspMarketData->SaleNumber6);
                            if (price > MINPRICE && volume > 0) {
                                _tmp_qtick.ap.push_back(price);
                                _tmp_qtick.av.push_back(volume);
                                price = atof(pRspMarketData->SalePrice7);
                                volume = atoll(pRspMarketData->SaleNumber7);
                                if (price > MINPRICE && volume > 0) {
                                    _tmp_qtick.ap.push_back(price);
                                    _tmp_qtick.av.push_back(volume);
                                    price = atof(pRspMarketData->SalePrice8);
                                    volume = atoll(pRspMarketData->SaleNumber8);
                                    if (price > MINPRICE && volume > 0) {
                                        _tmp_qtick.ap.push_back(price);
                                        _tmp_qtick.av.push_back(volume);
                                        price = atof(pRspMarketData->SalePrice9);
                                        volume = atoll(pRspMarketData->SaleNumber9);
                                        if (price > MINPRICE && volume > 0) {
                                            _tmp_qtick.ap.push_back(price);
                                            _tmp_qtick.av.push_back(volume);
                                            price = atof(pRspMarketData->SalePrice10);
                                            volume = atoll(pRspMarketData->SaleNumber10);
                                            if (price > MINPRICE && volume > 0) {
                                                _tmp_qtick.ap.push_back(price);
                                                _tmp_qtick.av.push_back(volume);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /*_tmp_qtick.ap = { 1, 2, 3, 6, 5, 7 };
        _tmp_qtick.av = { 1, 2, 3, 4, 5, 6 };*/
        // 检查ap的有效性
        if (_tmp_qtick.ap.size() > 1) {
            double pre_ap = _tmp_qtick.ap[0];
            size_t index = 1;
            for (; index < _tmp_qtick.ap.size(); index++) {
                if (_tmp_qtick.ap[index] < pre_ap) {
                    __error << "ask price change small, now: " << _tmp_qtick.ap[index]
                        << ", pre: " << pre_ap;
                    break;
                }else {
                    pre_ap = _tmp_qtick.ap[index];
                }
            }
            if (index < _tmp_qtick.ap.size()) {
                vector<double> ap;
                ap.swap(_tmp_qtick.ap);
                _tmp_qtick.ap.assign(ap.begin(), ap.begin() + index);

                vector<int64_t> av;
                av.swap(_tmp_qtick.av);
                _tmp_qtick.av.assign(av.begin(), av.begin() + index);
            }
        }

        if (_tmp_qtick.open < EPSILON && _tmp_qtick.bp.size() == 0 && _tmp_qtick.ap.size() == 0) {
            // __error << pRspMarketData->TreatyCode << ", " << pRspMarketData->ExchangeCode << ", open price: " << _tmp_qtick.open;
            return;
        }

        _tmp_qtick.status = kStateOK;
        string _line = _ctx->FinishQTick();
        QServer::Instance()->PushQTick(_line);
        // 1分钟更新一次
        if (pre_timestamp_ < _tmp_qtick.timestamp - 100000) {
            __info << "reset pre timestamp: " << pre_timestamp_ << ", now: " << _tmp_qtick.timestamp;
            pre_timestamp_ = _tmp_qtick.timestamp;
        }
    }
}  // namespace co