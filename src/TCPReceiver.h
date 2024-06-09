#ifndef SPW_ETHERNET_PRODUCER_TCPRECEIVER_H
#define SPW_ETHERNET_PRODUCER_TCPRECEIVER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include "logging.h"

using boost::asio::ip::tcp;

class TCPReceiver {
public:
    TCPReceiver(boost::asio::io_context& io_context, short port)
            : io_context_(io_context),
              acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
              buffer_(sizeof(size_t)) {
        StartAccept();
    }

    ~TCPReceiver() {
        acceptor_.cancel();
        io_context_.stop();
    }

protected:
    std::vector<char> buffer_;
    size_t expected_message_size_ = 0;
    size_t total_received_ = 0;

    void HandleReceiveSize(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<tcp::socket> socket) {
        if (!ec && bytes_transferred == sizeof(size_t)) {
            std::memcpy(&expected_message_size_, buffer_.data(), sizeof(size_t));
            buffer_.resize(expected_message_size_);
            total_received_ = 0;
            BOOST_LOG_TRIVIAL(info) << "New message size received: " << expected_message_size_;

            StartReceiveMessage(socket);
        } else {
            std::cerr << "Failed to receive size: " << ec.message() << std::endl;
        }
    }

    virtual void HandleMessage(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<tcp::socket> socket) {
        std::cout << "Received complete message \n";
        std::cout << std::endl;
    }

    void HandleReceiveMessage(const boost::system::error_code& ec, std::size_t bytes_transferred, std::shared_ptr<tcp::socket> socket) {
        if (!ec) {
            total_received_ += bytes_transferred;
            BOOST_LOG_TRIVIAL(info) << "Received " << bytes_transferred << " bytes, total received: " << total_received_;
            buffer_.resize(total_received_);

            if (total_received_ < expected_message_size_) {
                // Continue receiving the remaining data
                StartReceiveMessage(socket);
            } else {
                // All data received
                BOOST_LOG_TRIVIAL(info) << "All msg received";
                HandleMessage(ec, total_received_, socket);

                // Ready to receive the next message size
                StartReceiveSize(std::move(socket));
            }
        } else {
            std::cerr << "Failed to receive message: " << ec.message() << std::endl;
        }
    }

private:
    void StartAccept() {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec) {
            if (!ec) {
                StartReceiveSize(socket);
            }
            StartAccept();
        });
    }

    void StartReceiveSize(std::shared_ptr<tcp::socket> socket) {
        buffer_.resize(sizeof(size_t));
        socket->async_receive(boost::asio::buffer(buffer_),
                              std::bind(&TCPReceiver::HandleReceiveSize, this, std::placeholders::_1, std::placeholders::_2, socket));
    }

    void StartReceiveMessage(std::shared_ptr<tcp::socket> socket) {
        buffer_.resize(expected_message_size_);
        socket->async_receive(boost::asio::buffer(buffer_.data() + total_received_, expected_message_size_ - total_received_),
                              std::bind(&TCPReceiver::HandleReceiveMessage, this, std::placeholders::_1, std::placeholders::_2, socket));
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};

#endif //SPW_ETHERNET_PRODUCER_TCPRECEIVER_H
