//
// Created by oleg on 23.05.24.
//

#include "TCPReceiverForSPW.h"
#include "SpW_Eth_utils.h"
#include "logging.h"
#include "condition_variable"

void TCPReceiverForSPW::HandleMessage(const boost::system::error_code &ec, std::size_t bytes_transferred,
                                      std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    BOOST_LOG_TRIVIAL(debug) << "TCPReceiverForSPW::HandleReceiveMessage" << buffer_.size();
    if (!ec && bytes_transferred > 0) {

        std::unique_lock<std::mutex> lock(mu_);
        BOOST_LOG_TRIVIAL(debug) << "TCPReceiverForSPW::HandleReceiveMessage, !ec && bytes_transferred > 0";
        char *buf = buffer_.data();
        BOOST_LOG_TRIVIAL(info) << "Sending spw loopback message: " << bytes_transferred;
        long long before_recv = sync_spw_->acks_;

        SpW_send::send_data_loopback_debug(
                spw_socket_,
                (unsigned char *) buf,
                (const int) buffer_.size(),
                !ec && bytes_transferred
        );
        BOOST_LOG_TRIVIAL(debug) << "join on lock(mu_)";
        cv_.wait(lock, [before_recv, this] {
            BOOST_LOG_TRIVIAL(debug) << "Acks are: " << sync_spw_->acks_ << " and required >" << before_recv;
            return sync_spw_->acks_ != before_recv;
        });
        BOOST_LOG_TRIVIAL(debug) << "Ack joined";
    }
}

TCPReceiverForSPW::TCPReceiverForSPW(boost::asio::io_context& io_context, short port, std::string& default_spw_device)
:
TCPReceiver(io_context, port) {
    char* spw_device = const_cast<char*>(default_spw_device.c_str());
    spw_socket_ = SpW_connect::connect_debug_sender(spw_device);
    if (spw_socket_ < 0) {
        throw std::runtime_error("SpW bridge socket initialization error");
    }
    spw_eth_conf_header_2* conf_header = SpW_conf::create_conf_producer_packet(spw_device);
    auto res = SpW_conf::send_conf_packet_debug(*conf_header, spw_socket_);
    BOOST_LOG_TRIVIAL(info) << "Result conf packet send is: " << res;


    sync_ctx_ = new boost::asio::io_context;
    sync_spw_ = new TCPReceiverSyncSPW(*sync_ctx_, short(9005), cv_);
    std::thread([this]() {
        sync_ctx_->run();
    }).detach();
}

TCPReceiverForSPW::~TCPReceiverForSPW() {
    if (spw_socket_ > 0) {
        SpW_close::close_debug(spw_socket_);
    }
    sync_ctx_->stop();
    delete[] sync_ctx_;
}
