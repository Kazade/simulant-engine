#include <kazbase/logging.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>

#include "./debug_service.h"

namespace kglt {

void DebugService::start() {
    if(!start_server()) {
        return;
    }

    running_ = true;

    handle_ = std::async(std::launch::async, std::bind(&DebugService::run, this));
}

void DebugService::stop() {
    running_ = false;
    handle_.wait();
}

void DebugService::run() {
    struct sockaddr_in clientaddr;
    socklen_t addrlen;

    int slot = 0;

    // Signify there is no connection in each slot
    for(uint32_t i = 0; i < MAX_CLIENTS; ++i) {
        clients_[i] = -1;
    }

    while(running_) {
        addrlen = sizeof(clientaddr);
        clients_[slot] = accept(
            listen_fd_, (struct sockaddr*) &clientaddr, &addrlen
        );

        if(clients_[slot] < 0) {
            L_WARN("DebugService: error with connection");
        } else {
            // Start a thread to respond to the client
            std::async(std::launch::async, [=]() {
                respond(slot);
            });
        }
    }
}

bool DebugService::start_server() {
    struct addrinfo hints = {0};
    struct addrinfo *res = nullptr;
    struct addrinfo *p = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(nullptr, std::to_string(PORT).c_str(), &hints, &res) != 0) {
        L_ERROR("DebugService: Unable to start the debug service");
        return false;
    }

    for(p = res; p != nullptr; p = p->ai_next) {
        listen_fd_ = socket(p->ai_family, p->ai_socktype, 0);
        if(listen_fd_ == -1) continue;
        if(bind(listen_fd_, p->ai_addr, p->ai_addrlen) == 0) break;
    }

    if(!p) {
        L_ERROR("DebugService: Error binding to socket");
        return false;
    }

    freeaddrinfo(res);

    if(listen(listen_fd_, 1000000) != 0) {
        L_ERROR("DebugService: listen() error");
        return false;
    }

    return true;
}

void DebugService::respond(int client_id) {
    const int MAX_LENGTH = 1024 * 1024;
    char msg[MAX_LENGTH] = {0};

    int rcvd = recv(clients_[client_id], msg, MAX_LENGTH, 0);
    if(rcvd < 0) {
        L_WARN("DebugService: Error receiving from client");
    } else if(rcvd == 0) {
        L_WARN("DebugService: Client disconnected");
    } else {
        // Handle message
    }

    shutdown(clients_[client_id], SHUT_RDWR);
    close(clients_[client_id]);
    clients_[client_id] = -1;
}

}
