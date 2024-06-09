//
// Created by konin on 04.06.24.
//

#ifndef SPW_ETHERNET_PRODUCER_SPWRECEIVERFORTCP_H
#define SPW_ETHERNET_PRODUCER_SPWRECEIVERFORTCP_H
#include "SPWReceiver.h"
#include "ResilientSocket.h"

class SPWReceiverForTCP: public SPWReceiver {
public:
    explicit SPWReceiverForTCP(std::string &device, std::string& host, short port);
    ~SPWReceiverForTCP();
protected:
    void HandleReceive(std::vector<unsigned char>& message) override;
private:
    ResilientSocket* socket_publisher_;
};


#endif //SPW_ETHERNET_PRODUCER_SPWRECEIVERFORTCP_H
