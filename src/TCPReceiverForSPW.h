//
// Created by oleg on 23.05.24.
//

#ifndef SPW_ETHERNET_PRODUCER_TCPRECEIVERFORSPW_H
#define SPW_ETHERNET_PRODUCER_TCPRECEIVERFORSPW_H
#include "TCPReceiver.h"
#include "TCPReceiverSyncSPW.h"
#include "mutex"

class TCPReceiverForSPW: public TCPReceiver {
public:
    explicit TCPReceiverForSPW(boost::asio::io_context& io_context, short port, std::string& default_spw_device);
    ~TCPReceiverForSPW();
protected:
    void HandleMessage(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<tcp::socket> socket) override;
private:
    int spw_socket_;
    std::mutex mu_;
    TCPReceiverSyncSPW* sync_spw_;
    boost::asio::io_context* sync_ctx_;
    std::condition_variable cv_;
};


#endif //SPW_ETHERNET_PRODUCER_TCPRECEIVERFORSPW_H
