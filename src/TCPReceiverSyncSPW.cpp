//
// Created by oleg on 09.06.24.
//

#include "TCPReceiverSyncSPW.h"
#include "logging.h"

void TCPReceiverSyncSPW::HandleMessage(const boost::system::error_code &ec, std::size_t bytes_transferred,
                                       std::shared_ptr<tcp::socket> socket) {
    std::lock_guard<std::mutex> lock(mu_); // protect acks_ from race conditions
    acks_++;
    BOOST_LOG_TRIVIAL(info) << "ack received, acked packages: " << acks_;
    cv_->notify_all();
}

TCPReceiverSyncSPW::TCPReceiverSyncSPW(boost::asio::io_context& context, short port, std::condition_variable& cv): TCPReceiver(context, port) {
    acks_ = 0;
    cv_ = &cv;
}
