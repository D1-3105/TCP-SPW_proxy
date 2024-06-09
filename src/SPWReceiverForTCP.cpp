//
// Created by konin on 04.06.24.
//

#include "SPWReceiverForTCP.h"
#include "logging.h"

void SPWReceiverForTCP::HandleReceive(std::vector<unsigned char> &message) {
    bool is_published = false;
    if (message.size())
        is_published = socket_publisher_->publish(message);

    if (not is_published) {
        BOOST_LOG_TRIVIAL(info) << "packet with size " << message.size() << " ignored";
    } else {
        BOOST_LOG_TRIVIAL(debug) << "packet with size " << message.size() << " sent";
    }
}

SPWReceiverForTCP::SPWReceiverForTCP(std::string &device, std::string& host, short port) : SPWReceiver(device) {
    socket_publisher_ = new ResilientSocket();
    socket_publisher_->init_socket(host, port);
}

SPWReceiverForTCP::~SPWReceiverForTCP() {
    socket_publisher_->close_socket();
    delete socket_publisher_;
}
