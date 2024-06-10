#include <iostream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include "chrono"

std::chrono::high_resolution_clock::time_point send_time;

void* send_message(void* arg) {
    std::string host = "127.0.0.1";
    short port = 6300;
    std::string message;

    for (int i = 0; i < 2764828; i++)
        message += "1";

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int activity = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string input;
            std::getline(std::cin, input);
            if (!input.empty()) {
                message = input;
            }

            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);

            if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
                perror("inet_pton");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("connect");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            size_t message_size = message.size();
            std::vector<char> buffer(sizeof(size_t) + message_size);
            std::memcpy(buffer.data(), &message_size, sizeof(size_t)); // Copy the size of the message
            std::memcpy(buffer.data() + sizeof(size_t), message.data(), message_size); // Copy the message data

            send_time = std::chrono::high_resolution_clock::now(); // Mark send time

            if (send(sockfd, buffer.data(), buffer.size(), 0) < 0) {
                perror("send");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            std::cout << "Sent message of size " << message_size << std::endl;

            close(sockfd);
        }
    }
    return nullptr;
}

void* receive_message(void* arg) {
    short port = 6301;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 1) < 0) {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        int client_sockfd = accept(sockfd, nullptr, nullptr);
        if (client_sockfd < 0) {
            perror("accept");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        while (true) {
            size_t message_size;
            ssize_t recv_size = recv(client_sockfd, &message_size, sizeof(size_t), 0);
            if (recv_size <= 0 || message_size > 10000000) {
                continue;
            }

            std::vector<unsigned char> buffer(message_size);
            recv_size = recv(client_sockfd, buffer.data(), message_size, 0);
            if (recv_size <= 0) {
                continue;
            }

            auto receive_time = std::chrono::high_resolution_clock::now(); // Mark receive time
            std::chrono::duration<double> elapsed = receive_time - send_time;

            std::string received_message(buffer.begin(), buffer.end());
            std::cout << "Received message of size " << message_size << std::endl;
            std::cout << "Time between send and receive: " << elapsed.count() << " seconds" << std::endl;
        }

        close(client_sockfd);
    }

    close(sockfd);
    return nullptr;
}


int main() {
    pthread_t sender_thread, receiver_thread;

    if (pthread_create(&sender_thread, nullptr, send_message, nullptr) != 0) {
        perror("pthread_create (sender)");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&receiver_thread, nullptr, receive_message, nullptr) != 0) {
        perror("pthread_create (receiver)");
        exit(EXIT_FAILURE);
    }

    pthread_join(sender_thread, nullptr);
    pthread_join(receiver_thread, nullptr);

    return 0;
}
