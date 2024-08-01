// Copyright 2020 Fancapital Inc.  All rights reserved.
#include "x/x.h"
#include "./define.h"
#include "./config.h"
#include "./daapi_server.h"
#include <boost/program_options.hpp>

using co::Config;
using co::DaapiServer;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("[daapi_feeder] Usage");
        desc.add_options()
            ("help,h", "show help message")
            ("version,v", "show version information")
            ("passwd", po::value<std::string>(), "encode plain password");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.count("passwd")) {
            cout << co::EncodePassword(vm["passwd"].as<std::string>()) << endl;
            return 0;
        } else if (vm.count("help")) {
            cout << desc << endl;
            return 0;
        } else if (vm.count("version")) {
            cout << co::kVersion << endl;
            return 0;
        }

        __info << "start daapi_feeder: version = " << co::kVersion << " ......";
        Singleton<Config>::Instance();
        Singleton<Config>::GetInstance()->Init();

        DaapiServer server;
        server.Run();

        __info << "server is stopped.";
    } catch (x::Exception& e) {
        __fatal << "server is crashed, " << e.what();
        return 1;
    } catch (std::exception& e) {
        __fatal << "server is crashed, " << e.what();
        return 2;
    } catch (...) {
        __fatal << "server is crashed, unknown reason";
        return 3;
    }
    return 0;
}

