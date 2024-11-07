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
    MDBuffer::MDBuffer(shared_ptr<FeedServer> feeder_server): feeder_server_(feeder_server) {
        for (int _i = 0; _i < BUFFERLENGTH; _i++) {
            memset(buffer_ + _i, 0, sizeof(CMarketRspMarketDataField));
        }
        init_day_ = x::RawDate();
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
                    LOG_INFO << " read_index: " << _read_index
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

    void MDBuffer::AddMdbuffer(CMarketRspMarketDataField* pRspMarketData) {
        /*if (strcmp(pRspMarketData->TreatyCode, "CN2110") == 0) {
            pRspMarketData->Time[9] = '5';
            LOG_INFO << ", " << pRspMarketData->TreatyCode
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
            LOG_INFO << "write_index: " << write_index_.load()
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
        memcpy(buffer_ + mod_index, pRspMarketData, sizeof(CMarketRspMarketDataField));
        write_index_.add(1);
    }

    class PreData : public FeedUserData {
     public:
        int64_t timestamp = 0;
        int64_t sum_amount = 0;
        int64_t sum_volume = 0;
    };

    void MDBuffer::ProcessSnapshot(CMarketRspMarketDataField* pRspMarketData) {
        if (strlen(pRspMarketData->FilledNum) == 0) {
            return;
        }
//        if (strcmp(pRspMarketData->TreatyCode, "NQ2409") == 0) {
//            LOG_INFO << pRspMarketData->TreatyCode
//                     << ", " << pRspMarketData->ExchangeCode
//                     << ", Open: " << pRspMarketData->Open
//                     << ", CurrPrice: " << pRspMarketData->CurrPrice
//                     << ", CurrNumber: " << pRspMarketData->CurrNumber
//                     << ", FilledNum: " << pRspMarketData->FilledNum
//                     << ", HoldNum: " << pRspMarketData->HoldNum
//                     << ", Time: " << pRspMarketData->Time
//                     << ", BuyPrice: " << pRspMarketData->BuyPrice
//                     << ", BuyNumber: " << pRspMarketData->BuyNumber
//                     << ", SalePrice: " << pRspMarketData->SalePrice
//                     << ", SaleNumber: " << pRspMarketData->SaleNumber
//                     ;
//        } else {
//            return;
//        }
        int64_t market = 0;
        string exchange = x::Trim(pRspMarketData->ExchangeCode);
        if (exchange == "SGXQ") {
            market = kMarketSGX;
        } else if (exchange == "HKEX") {
            market = kMarketHK;
        } else if (exchange == "CME") {
            market = kMarketCME;
        } else if (exchange == "CME_CBT") {
            market = kMarketCMECBT;
        } else if (exchange == "NYBOT") {
            market = kMarketNYBOT;
        } else if (exchange == "LME") {
            market = kMarketLME;
        } else if (exchange == "ICE") {
            market = kMarketICE;
        } else if (exchange == "XEurex") {
            market = kMarketXEUREX;
        } else if (exchange == "TOCOM") {
            market = kMarketTOCOM;
        } else if (exchange == "KRX") {
            market = kMarketKRX;
        } else {
            LOG_INFO << "not valid code: " << pRspMarketData->TreatyCode << ", exchange: " << exchange;
            return;
        }
        string std_code = x::Trim(pRspMarketData->TreatyCode) + string(MarketToSuffix(market).data());
        FeedContext* context = nullptr;
        MemQTick* tick = feeder_server_->CreateQTick(std_code.c_str(), &context);
        if (!tick || context->GetQContract()->dtype <= 0) {
            MemQContract *contract = feeder_server_->CreateQContract(std_code.c_str(), &context);
            if (!contract) {
                return;
            }
            memcpy(contract, context->GetQContract(), sizeof(MemQContract));
            strncpy(contract->code, std_code.c_str(), std_code.length());
            contract->timestamp = x::RawDate() * 1000000000LL;
            contract->market = market;
            contract->dtype = kDTypeFuture;
            double temp_price = atof(pRspMarketData->PreSettlementPrice);
            if (temp_price < MAXPIRCE) {
                contract->pre_close = temp_price;
            }
            temp_price = 0;
            temp_price = atof(pRspMarketData->LimitUpPrice);
            if (temp_price < MAXPIRCE) {
                contract->upper_limit = temp_price;
            }
            temp_price = 0;
            temp_price = atof(pRspMarketData->LimitDownPrice);
            if (temp_price < MAXPIRCE) {
                contract->lower_limit = temp_price;
            }
//            auto user_data = context->user_data();
//            PreData* pre_data = nullptr;
//            if (user_data == nullptr) {
//                pre_data = new PreData();
//                pre_data->sum_amount = 0;
//                pre_data->sum_volume = 0;
//                context->set_user_data(static_cast<FeedUserData*>(pre_data));
//            } else {
//                pre_data = static_cast<PreData*>(user_data);
//            }
            feeder_server_->PushQContract(context, contract);
            tick = feeder_server_->CreateQTick(std_code.c_str(), &context);
            if (!tick) {
                return;
            }
        }

        string str_timestamp;
        for (int Index = 0; Index < static_cast<int>(strlen(pRspMarketData->Time)); ++Index) {
            if (pRspMarketData->Time[Index] >= '0' && pRspMarketData->Time[Index] <= '9') {
                str_timestamp += pRspMarketData->Time[Index];
            }
        }
        int tick_day = atoll(str_timestamp.c_str()) / 1000000LL;

        if (tick_day < init_day_) {
            LOG_ERROR << "not valid time day: " << tick_day;
            return;
        }
        int64_t timestamp = atoll(str_timestamp.c_str()) * 1000LL;

        tick->src = 0;
        strncpy(tick->code, std_code.c_str(), std_code.length());
        tick->timestamp = timestamp;
        tick->new_price = atof(pRspMarketData->CurrPrice);
        // tick->new_volume = atoll(pRspMarketData->CurrNumber);
        tick->sum_volume = atoll(pRspMarketData->FilledNum);
        tick->open_interest = atoll(pRspMarketData->HoldNum);

        double _temp_price = atof(pRspMarketData->Open);
        if (_temp_price < MAXPIRCE) {
            tick->open = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->High);
        if (_temp_price < MAXPIRCE) {
            tick->high = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->Low);
        if (_temp_price < MAXPIRCE) {
            tick->low = _temp_price;
        }

        _temp_price = 0;
        _temp_price = atof(pRspMarketData->Close);
        if (_temp_price < MAXPIRCE) {
            tick->close = _temp_price;
        }
        double price = 0.0;
        int64_t volume = 0;
        price = atof(pRspMarketData->BuyPrice);
        volume = atoll(pRspMarketData->BuyNumber);
        if (price > MINPRICE && volume > 0) {
            tick->bp[0] = price;
            tick->bv[0] = volume;
            price = atof(pRspMarketData->BuyPrice2);
            volume = atoll(pRspMarketData->BuyNumber2);
            if (price > MINPRICE && volume > 0) {
                tick->bp[1] = price;
                tick->bv[1] = volume;
                price = atof(pRspMarketData->BuyPrice3);
                volume = atoll(pRspMarketData->BuyNumber3);
                if (price > MINPRICE && volume > 0) {
                    tick->bp[2] = price;
                    tick->bv[2] = volume;
                    price = atof(pRspMarketData->BuyPrice4);
                    volume = atoll(pRspMarketData->BuyNumber4);
                    if (price > MINPRICE && volume > 0) {
                        tick->bp[3] = price;
                        tick->bv[3] = volume;
                        price = atof(pRspMarketData->BuyPrice5);
                        volume = atoll(pRspMarketData->BuyNumber5);
                        if (price > MINPRICE && volume > 0) {
                            tick->bp[4] = price;
                            tick->bv[4] = volume;
                            price = atof(pRspMarketData->BuyPrice6);
                            volume = atoll(pRspMarketData->BuyNumber6);
                            if (price > MINPRICE && volume > 0) {
                                tick->bp[5] = price;
                                tick->bv[5] = volume;
                                price = atof(pRspMarketData->BuyPrice7);
                                volume = atoll(pRspMarketData->BuyNumber7);
                                if (price > MINPRICE && volume > 0) {
                                    tick->bp[6] = price;
                                    tick->bv[6] = volume;
                                    price = atof(pRspMarketData->BuyPrice8);
                                    volume = atoll(pRspMarketData->BuyNumber8);
                                    if (price > MINPRICE && volume > 0) {
                                        tick->bp[7] = price;
                                        tick->bv[7] = volume;
                                        price = atof(pRspMarketData->BuyPrice9);
                                        volume = atoll(pRspMarketData->BuyNumber9);
                                        if (price > MINPRICE && volume > 0) {
                                            tick->bp[8] = price;
                                            tick->bv[8] = volume;
                                            price = atof(pRspMarketData->BuyPrice10);
                                            volume = atoll(pRspMarketData->BuyNumber10);
                                            if (price > MINPRICE && volume > 0) {
                                                tick->bp[9] = price;
                                                tick->bv[9] = volume;
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

        price = atof(pRspMarketData->SalePrice);
        volume = atoll(pRspMarketData->SaleNumber);
        if (price > MINPRICE && volume > 0) {
            tick->ap[0] = price;
            tick->av[0] = volume;
            price = atof(pRspMarketData->SalePrice2);
            volume = atoll(pRspMarketData->SaleNumber2);
            if (price > MINPRICE && volume > 0) {
                tick->ap[1] = price;
                tick->av[1] = volume;
                price = atof(pRspMarketData->SalePrice3);
                volume = atoll(pRspMarketData->SaleNumber3);
                if (price > MINPRICE && volume > 0) {
                    tick->ap[2] = price;
                    tick->av[2] = volume;
                    price = atof(pRspMarketData->SalePrice4);
                    volume = atoll(pRspMarketData->SaleNumber4);
                    if (price > MINPRICE && volume > 0) {
                        tick->ap[3] = price;
                        tick->av[3] = volume;
                        price = atof(pRspMarketData->SalePrice5);
                        volume = atoll(pRspMarketData->SaleNumber5);
                        if (price > MINPRICE && volume > 0) {
                            tick->ap[4] = price;
                            tick->av[4] = volume;
                            price = atof(pRspMarketData->SalePrice6);
                            volume = atoll(pRspMarketData->SaleNumber6);
                            if (price > MINPRICE && volume > 0) {
                                tick->ap[5] = price;
                                tick->av[5] = volume;
                                price = atof(pRspMarketData->SalePrice7);
                                volume = atoll(pRspMarketData->SaleNumber7);
                                if (price > MINPRICE && volume > 0) {
                                    tick->ap[6] = price;
                                    tick->av[6] = volume;
                                    price = atof(pRspMarketData->SalePrice8);
                                    volume = atoll(pRspMarketData->SaleNumber8);
                                    if (price > MINPRICE && volume > 0) {
                                        tick->ap[7] = price;
                                        tick->av[7] = volume;
                                        price = atof(pRspMarketData->SalePrice9);
                                        volume = atoll(pRspMarketData->SaleNumber9);
                                        if (price > MINPRICE && volume > 0) {
                                            tick->ap[8] = price;
                                            tick->av[8] = volume;
                                            price = atof(pRspMarketData->SalePrice10);
                                            volume = atoll(pRspMarketData->SaleNumber10);
                                            if (price > MINPRICE && volume > 0) {
                                                tick->ap[9] = price;
                                                tick->av[9] = volume;
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
        tick->state = kStateOK;
        feeder_server_->PushQTick(context, tick);
    }
}  // namespace co