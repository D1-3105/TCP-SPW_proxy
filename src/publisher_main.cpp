//
// Created by oleg on 14.05.24.
//
#include "TCPReceiverForSPW.h"
#include "iostream"
#include "logging.h"


void handle_sigint(int sig) {
    std::cout << "\nCaught signal " << sig << ", shutting down...\n";
    exit(sig);
}


int main()
{
    logging::init_logging();
    signal(SIGINT, handle_sigint);
    char* device = getenv("SPW_BRIDGE_DEVICE");
    if (not device) {
        device = (char*) std::string("eno1").c_str();
    }
    try{
        boost::asio::io_context io_context;
        std::string stringed_device = std::string(device);
        auto receiver = TCPReceiverForSPW(io_context, 6300, stringed_device);
        io_context.run();
    } catch (std::runtime_error& re) {
        BOOST_LOG_TRIVIAL(error) << re.what();
    }

    return 0;
}
