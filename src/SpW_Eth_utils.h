//
// Created by oleg on 23.05.24.
//

#ifndef SPW_ETHERNET_PRODUCER_SPW_ETH_UTILS_H
#define SPW_ETHERNET_PRODUCER_SPW_ETH_UTILS_H
#include "spw_eth.h"
#include "TCPReceiverSyncSPW.h"
#include <netinet/ether.h>


namespace utils {
    void my_mac(ether_addr*& e, const char* device);
    std::string get_my_mac(const char* interfaceName);
}

namespace SpW_connect {
    int connect(char* device);

    int connect_debug_sender(char* device);

    int connect_debug_consumer(char* device);
}

namespace SpW_recv {
    int recv(int socket, unsigned char* buf, int buf_size, unsigned char*, unsigned char*);

    int recv_debug(int s, unsigned char* buf, int buf_size, unsigned char*, unsigned char*);
}

namespace SpW_conf {
    int send_conf_packet(spw_eth_conf_header_2 str, unsigned rawsock);
    int send_conf_packet_debug( spw_eth_conf_header_2 str, unsigned rawsock);

    spw_eth_conf_header_2* create_conf_producer_packet(const char* device);
}


namespace SpW_close {
    int close(int s);
    int close_debug(int s);
}

namespace SpW_send {
    void send_data_loopback(TCPReceiverSyncSPW& syncer, const int &socket, unsigned char *buf, const int& buf_size, const bool& is_error);

    void send_data_loopback_debug(TCPReceiverSyncSPW& syncer, const int &socket, unsigned char *buf, const int& buf_size, const bool& is_error);
}

#endif //SPW_ETHERNET_PRODUCER_SPW_ETH_UTILS_H
