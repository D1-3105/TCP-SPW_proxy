//
// Created by konin on 31.05.24.
//
#include "SPWReceiver.h"
#include "vector"
#include "logging.h"
#include "SpW_Eth_utils.h"



void SPWReceiver::Receive() {
    const size_t packet_size = 1500;
    auto* buf = new unsigned char[packet_size];
    unsigned char end_packet;
    auto* mac = new ether_addr;
    utils::my_mac(mac, static_cast<const char*>(device_.c_str()));

    size_t total_size = 0;
    size_t received_bytes = 0;
    std::vector<unsigned char> all_buf;
    std::vector<int> received_packet_numbers;

    BOOST_LOG_TRIVIAL(debug) << "SpW_Recv_Packet_From_MAC start";
    BOOST_LOG_TRIVIAL(debug) << "MY MAC: " << ether_ntoa(mac);

    // Receive the first packet to get the total size
    int res = SpW_recv::recv(spw_socket_, buf, packet_size, (unsigned char*) mac->ether_addr_octet, &end_packet);
    if (res < 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to receive initial packet: " << strerror(errno);
        delete[] buf;
        return;
    }
    SendPAck();
    BOOST_LOG_TRIVIAL(debug) << "SpW_Recv_Packet_From_MAC end";

    // Extract the total size and packet number from the first packet
    int packet_num = *(int*)buf;
    total_size = *(int*)(buf + sizeof(int));
    all_buf.resize(total_size);

    // Calculate the size of the data in the first packet
    size_t first_packet_data_size = packet_size - 2 * sizeof(int);
    if (total_size < first_packet_data_size) {
        first_packet_data_size = total_size;
    }

    // Copy data from the first packet
    std::memcpy(all_buf.data(), buf + 2 * sizeof(int), first_packet_data_size);
    received_bytes += first_packet_data_size;
    received_packet_numbers.push_back(packet_num);

    // Receive the remaining packets
    while (received_bytes < total_size) {
        BOOST_LOG_TRIVIAL(debug) << "Receive packet: " << received_bytes << "/" << total_size;

        res = SpW_recv::recv(spw_socket_, buf, packet_size, (unsigned char*) mac->ether_addr_octet, &end_packet);
        SendPAck();
        if (res < 0) {
            BOOST_LOG_TRIVIAL(error) << "Failed to receive packet: " << strerror(errno);
            return;
        }

        // Extract packet number
        packet_num = *(int*)buf;
        BOOST_LOG_TRIVIAL(debug) << "Packet " << packet_num << " received";
        // Calculate the size of data to copy from the current packet
        size_t current_packet_data_size = std::min(packet_size - sizeof(int), total_size - received_bytes);

        // Check if the packet number is already received
        if (std::find(received_packet_numbers.begin(), received_packet_numbers.end(), packet_num) == received_packet_numbers.end()) {
            // If not received, copy data from the current packet
            std::memcpy(all_buf.data() + received_bytes, buf + sizeof(int), current_packet_data_size);
            received_bytes += current_packet_data_size;
            received_packet_numbers.push_back(packet_num);
        } else {
            // If received, overwrite the corresponding part of the buffer
            std::memcpy(all_buf.data() + (packet_num * (packet_size - sizeof(int))), buf + sizeof(int), current_packet_data_size);
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "HandleReceive start with buf " << all_buf.size();
    HandleReceive(all_buf);
    BOOST_LOG_TRIVIAL(debug) << "HandleReceive end";

    // Cleanup
    delete[] buf;
    BOOST_LOG_TRIVIAL(debug) << "delete[] buf;";
}



SPWReceiver::SPWReceiver(std::string &device) {
    device_ = device;
    spw_socket_ = SpW_connect::connect((char*)device.c_str());
    if (spw_socket_ < 0) {
        throw std::runtime_error("Error during spw_socket_ creation");
    }
    ack_ = new ResilientSocket();
    ack_->init_socket("127.0.0.1", 9005);
}

void SPWReceiver::HandleReceive(std::vector<unsigned char> &message) {
    BOOST_LOG_TRIVIAL(info) << "Received new spw message: " << message.size();
}

[[noreturn]] void SPWReceiver::run() {
    while(true) {
        Receive();
        std::vector<unsigned char> ack_msg = {'O', 'K', '\0'};
        ack_->publish(ack_msg);
    }
}

SPWReceiver::~SPWReceiver() {
    if (spw_socket_ > 0) {
        SpW_close::close(spw_socket_);
    }
    ack_->close_socket();
    delete ack_;
}

void SPWReceiver::SendPAck() {
    const char* sync_msg = "PACK";
    auto sync_vector = std::vector<unsigned char>(sync_msg, sync_msg+5);
    ack_->publish(sync_vector);
};
