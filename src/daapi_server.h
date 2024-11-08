// Copyright 2020 Fancapital Inc.  All rights reserved.
#pragma once
#include "./md_callback.h"

namespace co {
class DaapiServer {
 public:
    DaapiServer();
    ~DaapiServer();
    void Run();

 private:
    vector<MDCallBack*> call_back_;
};
}  // namespace co
