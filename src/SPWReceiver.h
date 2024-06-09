//
// Created by konin on 31.05.24.
//

#ifndef SPW_ETHERNET_PRODUCER_SPWRECEIVER_H
#define SPW_ETHERNET_PRODUCER_SPWRECEIVER_H

#include <iostream>
#include "SpW_Eth_utils.h"
#include "ResilientSocket.h"
#include "vector"

class SPWReceiver {
public:
    explicit SPWReceiver(std::string& device);
    ~SPWReceiver();
    [[noreturn]] void run();
protected:
    virtual void HandleReceive(std::vector<unsigned char>& message);
private:
    std::string device_;

    ResilientSocket* ack_;

    int spw_socket_;
    void Receive();
};

#endif //SPW_ETHERNET_PRODUCER_SPWRECEIVER_H
