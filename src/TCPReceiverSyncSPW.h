//
// Created by oleg on 09.06.24.
//

#ifndef SPW_ETHERNET_PRODUCER_TCPRECEIVERSYNCSPW_H
#define SPW_ETHERNET_PRODUCER_TCPRECEIVERSYNCSPW_H

#include "TCPReceiver.h"
#include "mutex"
#include "condition_variable"

class TCPReceiverSyncSPW: public TCPReceiver {
public:
    explicit TCPReceiverSyncSPW(boost::asio::io_context& context, short port, std::condition_variable& cv);
    long long acks_;
    std::condition_variable cv_subseq_;
    std::mutex mu_subseq_;
private:
    std::vector<char> buffer_;
    std::mutex mu_;
    std::condition_variable* cv_;
protected:
    void HandleMessage(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<tcp::socket> socket) override;
};


#endif //SPW_ETHERNET_PRODUCER_TCPRECEIVERSYNCSPW_H
