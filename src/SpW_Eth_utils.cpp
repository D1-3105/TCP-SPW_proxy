//
// Created by oleg on 23.05.24.
//

#include <string>
#include <ifaddrs.h>
#include <vector>
#include <sstream>
#include <cmath>
#include <arpa/inet.h>
#include <condition_variable>

#include "SpW_Eth_utils.h"
#include "logging.h"

std::string utils::get_my_mac(const char* interfaceName) {
    struct ifaddrs* ifAddrStruct = nullptr;
    std::string macAddress;

    if (getifaddrs(&ifAddrStruct)!= -1) {
        for (struct ifaddrs* ifa = ifAddrStruct; ifa!= nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family!= AF_PACKET) {
                continue;
            }

            if (strcmp(ifa->ifa_name, interfaceName) == 0) {
                unsigned char* mac = ((struct sockaddr_ll*)ifa->ifa_addr)->sll_addr;
                char buffer[18]; // 17 characters for MAC + null terminator
                sprintf(
                        buffer,
                        "%02x:%02x:%02x:%02x:%02x:%02x",
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
                        );
                macAddress = buffer;
                break;
            }
        }
        freeifaddrs(ifAddrStruct);
    }

    return macAddress;
}

int SpW_conf::send_conf_packet(spw_eth_conf_header_2 str, unsigned int rawsock) {
    int result, i;
    result = SpW_Eth_Send_Conf_Packet(rawsock, str);
    if (result < 0) {
        throw std::runtime_error("SpW_Eth_Send_Conf_Packet - failed!");
    }
    return result;
}

int SpW_conf::send_conf_packet_debug(spw_eth_conf_header_2 str, unsigned int rawsock) {return 1;}

std::string mirror_mac(const std::string& mac) {
    std::istringstream iss(mac);
    std::string byte;
    std::vector<std::string> bytes;
    while (getline(iss, byte, ':')) {
        bytes.push_back(byte);
    }

    std::ostringstream oss;
    for (auto it = bytes.rbegin(); it!= bytes.rend(); ++it) {
        oss << *it;
        if (it + 1!= bytes.rend()) {
            oss << ":";
        }
    }

    return oss.str();
}


void utils::my_mac(ether_addr*& e, const char* device) {
    std::string my_mac = get_my_mac(device);
    e = ether_aton(my_mac.c_str());
}


spw_eth_conf_header_2* SpW_conf::create_conf_producer_packet(const char* device) {
    auto* config_data = new spw_eth_conf_header_2;
    memset(config_data, 0, SIZE_CONF_STRUCTURE);
    memcpy(&config_data->GE_SPW, &CONF_STRING, sizeof(CONF_STRING)); // just control value of conf
    config_data->edit0 = CONF_SET_SPEED;
    config_data->Spw_Speed = 1; /* установка скорости 400 Мбит/c */
    config_data->edit1 = CONF_SET_SWITCH_MODE | CONF_SET_STATUS_FREQ;
    config_data->stat_freq = 20; /* частота отправки состояния моста */
    config_data->switch_mode = 0x34;  /* data mode 0x4, ccode mode 0x3*/
    return config_data;
}


void SpW_send::send_data_loopback(const int &socket, unsigned char *buf, const int &buf_size, const bool& is_error) {
    int packet_end;

    // Calculate the number of packets
    size_t total_packets = ceil(double(buf_size + sizeof(int)) / 1500);
    size_t transported_bytes = 0;

    // Create buffer to hold packet size and data
    unsigned char temp_buf[1500]; // Maximum size of one packet, stack-allocated to avoid heap allocation issues

    for (size_t packet_num = 0; packet_num < total_packets; ++packet_num) {
        int current_packet_size;

        // If this is the first packet, add the total message size at the beginning
        if (packet_num == 0) {
            current_packet_size = std::min<int>(1500 - sizeof(int), buf_size - transported_bytes);
            *(int*)temp_buf = buf_size; // Write the total message size at the beginning of the first packet
            memcpy(temp_buf + sizeof(int), buf + transported_bytes, current_packet_size);
            current_packet_size += sizeof(int); // Adjust the packet size by the size of the written length information
        } else {
            // For subsequent packets, just copy the data
            current_packet_size = std::min<int>(1500, buf_size - transported_bytes);
            memcpy(temp_buf, buf + transported_bytes, current_packet_size);
        }

        // Determine the type of packet
        packet_end = SpW_Eth_EOF;
        // Send the packet
        int res = SpW_Send_Packet(socket, temp_buf, current_packet_size, packet_end);
        transported_bytes += current_packet_size - ((packet_num == 0) ? sizeof(int) : 0);
    }
}


void SpW_send::send_data_loopback_debug(const int &socket, unsigned char *buf, const int &buf_size, const bool& is_error) {
    // Calculate the number of packets
    size_t total_packets = ceil(double(buf_size + sizeof(int)) / 1500);
    size_t transported_bytes = 0;

    // Create buffer to hold packet size and data
    unsigned char temp_buf[1500]; // Maximum size of one packet, stack-allocated to avoid heap allocation issues

    for (size_t packet_num = 0; packet_num < total_packets; ++packet_num) {
        int current_packet_size;

        // If this is the first packet, add the total message size at the beginning
        if (packet_num == 0) {
            current_packet_size = std::min<int>(1500 - sizeof(int), buf_size - transported_bytes);
            *(int*)temp_buf = buf_size; // Write the total message size at the beginning of the first packet
            memcpy(temp_buf + sizeof(int), buf + transported_bytes, current_packet_size);
            current_packet_size += sizeof(int); // Adjust the packet size by the size of the written length information
        } else {
            // For subsequent packets, just copy the data
            current_packet_size = std::min<int>(1500, buf_size - transported_bytes);
            memcpy(temp_buf, buf + transported_bytes, current_packet_size);
        }

        // Send the packet
        int res = send(socket, temp_buf, current_packet_size, 0);
        if (res < 0) {
            std::cerr << "Failed to send packet: " << strerror(errno) << std::endl;
            return;
        }
        transported_bytes += current_packet_size - ((packet_num == 0) ? sizeof(int) : 0);
    }
}


short server_port = 9000;
auto* server_ip = "127.0.0.1";

int SpW_connect::connect_debug_sender(char* device) {

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Ошибка создания сокета: " << strerror(errno) << std::endl;
        return -1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "Неверный адрес: " << server_ip << std::endl;
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка подключения: " << strerror(errno) << std::endl;
        close(sock);
        return -1;
    }

    return sock;
}


std::vector<std::vector<unsigned char>> packets;
std::mutex packets_mutex;
std::condition_variable packets_cv;

void* handle_client(void* client_sock) {
    int client_socket = *reinterpret_cast<int*>(client_sock);
    delete reinterpret_cast<int*>(client_sock);

    while (true) {
        unsigned char buffer[1500];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                std::cerr << "Ошибка получения данных: " << strerror(errno) << std::endl;
            }
            break;
        }

        {
            std::lock_guard<std::mutex> lock(packets_mutex);
            packets.push_back(std::vector<unsigned char>(buffer, buffer + bytes_received));
        }
        packets_cv.notify_one();
    }

    close(client_socket);
    return nullptr;
}

// Функция для принятия подключений в отдельном потоке
void* accept_connections(void* params) {
    int server_sock = *reinterpret_cast<int*>(params);

    while (true) {
        std::cout << "Ожидание подключения..." << std::endl;

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int* client_socket = new int;
        *client_socket = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (*client_socket < 0) {
            std::cerr << "Ошибка принятия подключения: " << strerror(errno) << std::endl;
            delete client_socket;
            continue;
        }

        std::cout << "Подключение установлено" << std::endl;

        pthread_t client_thread;
        if (pthread_create(&client_thread, nullptr, handle_client, client_socket) != 0) {
            std::cerr << "Ошибка создания потока для клиента: " << strerror(errno) << std::endl;
            close(*client_socket);
            delete client_socket;
        } else {
            pthread_detach(client_thread);
        }
    }

    close(server_sock);
    return nullptr;
}



int SpW_connect::connect_debug_consumer(char* device) {
    // Создаем сокет
    int* server_sock = new int(socket(AF_INET, SOCK_STREAM, 0));
    if (*server_sock < 0) {
        std::cerr << "Ошибка создания сокета: " << strerror(errno) << std::endl;
        delete server_sock;
        return -1;
    }

    // Заполняем структуру адреса сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Связываем сокет с адресом и портом
    if (bind(*server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка привязки: " << strerror(errno) << std::endl;
        close(*server_sock);
        delete server_sock;
        return -1;
    }

    // Начинаем прослушивание входящих подключений
    if (listen(*server_sock, 10) < 0) {
        std::cerr << "Ошибка прослушивания: " << strerror(errno) << std::endl;
        close(*server_sock);
        delete server_sock;
        return -1;
    }

    // Создаем и запускаем поток для принятия подключений
    pthread_t server_thread;
    if (pthread_create(&server_thread, nullptr, accept_connections, server_sock) != 0) {
        std::cerr << "Ошибка создания потока: " << strerror(errno) << std::endl;
        close(*server_sock);
        delete server_sock;
        return -1;
    }

    // Возвращаем дескриптор сокета
    return *server_sock;
}

int SpW_connect::connect(char* device) {
    return SpW_Socket_Init(device);
}


int SpW_recv::recv(int s, unsigned char* buf, int buf_size, unsigned char* mac, unsigned char *end_packet) {
    return SpW_Recv_Packet(s, buf, buf_size, mac, end_packet);
}


int SpW_recv::recv_debug(int s, unsigned char* buf, int buf_size, unsigned char*, unsigned char*) {
    std::unique_lock<std::mutex> lock(packets_mutex);
    packets_cv.wait(lock, [] { return !packets.empty(); });
    BOOST_LOG_TRIVIAL(debug) << "Packets: " << packets.size();
    if (packets.empty()) {
        std::cerr << "Ошибка: пакет не найден после пробуждения" << std::endl;
        return -1;
    }

    std::vector<unsigned char> packet = packets.front();
    packets.erase(packets.begin());

    int copy_size = std::min(static_cast<int>(packet.size()), buf_size);
    std::copy(packet.begin(), packet.begin() + copy_size, buf);

    return copy_size;
}


int SpW_close::close(int s){
    return SpW_Socket_Close(s);
}


int SpW_close::close_debug(int s){
    return close(s);
}


