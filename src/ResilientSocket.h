#ifndef SPW_ETHERNET_PRODUCER_RESILIENTSOCKET_H
#define SPW_ETHERNET_PRODUCER_RESILIENTSOCKET_H

#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/log/trivial.hpp>

class ResilientSocket {
private:
    int sockfd_;
    std::string host_;
    int port_;
    pthread_t daemon_thread_;
    bool stop_daemon_;
    bool daemon_thread_running_;

public:
    ResilientSocket() : sockfd_(-1), host_(""), port_(0), stop_daemon_(false), daemon_thread_running_(false) {}

    ~ResilientSocket() {
        close_socket();
    }

    bool init_socket(const std::string& host, int port) {
        close_socket();  // Ensure previous socket is closed before opening a new one

        host_ = host;
        port_ = port;

        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            BOOST_LOG_TRIVIAL(error) << "Failed to create socket";
            return false;
        }

        sockaddr_in server_addr{};
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            BOOST_LOG_TRIVIAL(error) << "Invalid address/ Address not supported";
            close_socket();
            return false;
        }

        if (connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            BOOST_LOG_TRIVIAL(error) << "Connection failed";
            close_socket();
            return false;
        }

        return true;
    }

    bool publish(const std::vector<unsigned char>& message) {
        if (!is_socket_alive()) {
            BOOST_LOG_TRIVIAL(info) << "Socket disconnected. Reinitializing...";
            if (!init_socket(host_, port_)) {
                BOOST_LOG_TRIVIAL(error) << "Reinitialization failed";
                return false;
            }
        }

        size_t total_bytes_sent = 0;
        size_t message_size = message.size();
        ssize_t bytes_size = send(sockfd_, &message_size, sizeof(size_t), 0);
        if (bytes_size < 0) {
            BOOST_LOG_TRIVIAL(error) << "Failed to send size: " << strerror(errno);
            return false;
        }
        const char* data = reinterpret_cast<const char*>(message.data());

        while (total_bytes_sent < message_size) {
            ssize_t bytes_sent = send(sockfd_, data + total_bytes_sent, message_size - total_bytes_sent, 0);
            if (bytes_sent < 0) {
                BOOST_LOG_TRIVIAL(error) << "Failed to send message: " << strerror(errno);
                return false;
            }
            total_bytes_sent += bytes_sent;
            BOOST_LOG_TRIVIAL(info) << "Sent bytes: " << bytes_sent << ", Total sent: " << total_bytes_sent << " of " << message_size;
        }

        return true;
    }

    void close_socket() {
        if (sockfd_ != -1) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }

private:

    bool is_socket_alive() const {
        char buffer;
        int result = recv(sockfd_, &buffer, 1, MSG_PEEK | MSG_DONTWAIT);
        if (result == 0) {
            return false; // Socket is closed
        } else if (result < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
            return false; // Socket error
        }
        return true;
    }
};

#endif //SPW_ETHERNET_PRODUCER_RESILIENTSOCKET_H
