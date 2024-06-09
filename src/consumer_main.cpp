//
// Created by konin on 03.06.24.
//

#include "SPWReceiverForTCP.h"
#include "logging.h"

int main() {
    logging::init_logging();
    std::string device(getenv("SPW_DEVICE"));
    char* send_to_host = getenv("SEND_TO_HOST");;

    if (!send_to_host) {
        send_to_host = "127.0.0.1";
    }

    short sent_to_port = 6301;
    std:: string host(send_to_host);
    SPWReceiverForTCP receiver(device, host, sent_to_port);
    receiver.run();
}
