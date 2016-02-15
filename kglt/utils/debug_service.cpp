#include <kazbase/logging.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>

#include "./debug_service.h"

namespace kglt {

DebugService::~DebugService() {
    stop();
}

void DebugService::start() {
    if(running_) {
        return;
    }

    if(!start_server()) {
        return;
    }

    handle_ = std::async(std::launch::async, std::bind(&DebugService::run, this));
}

void DebugService::stop() {
    running_ = false;
    if(handle_.valid()) {
        handle_.get();
    }
}

struct Always {
    Always(std::function<void ()> func):
        func_(func) {}

    ~Always() {
        func_();
    }
    std::function<void ()> func_;
};

void DebugService::run() {
    auto ready = [](int socket) -> bool {
        fd_set rfds;
        struct timeval tv;

        FD_ZERO(&rfds);
        FD_SET(socket, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        return select(socket + 1, &rfds, NULL, NULL, &tv);
    };

    running_ = true;

    struct sockaddr_in clientaddr;
    socklen_t addrlen;

    int slot = 0;

    // Signify there is no connection in each slot
    {
        std::lock_guard<std::mutex> lock(client_lock_);
        for(uint32_t i = 0; i < MAX_CLIENTS; ++i) {
            clients_[i] = -1;
        }
    }

    while(running_) {
        addrlen = sizeof(clientaddr);

        int new_socket = 0;
        if(ready(listen_fd_)) {
            std::lock_guard<std::mutex> lock(client_lock_);
            new_socket = clients_[slot] = accept(
                listen_fd_, (struct sockaddr*) &clientaddr, &addrlen
            );
            L_DEBUG("DebugService: Accepted client");
        } else {
            continue;
        }

        if(new_socket < 0) {
            L_WARN("DebugService: error with connection");
        } else {
            L_DEBUG("DebugService: Client connected");
            // Start a thread to respond to the client
            client_threads_[slot] = std::async(std::launch::async, [=]() {
                //(ab)use RAII to ensure no matter what happens we release the client
                Always __([=]() {
                    L_DEBUG("DebugService: Freeing client");

                    shutdown(new_socket, SHUT_RDWR);
                    close(new_socket);

                    std::lock_guard<std::mutex> lock(client_lock_);
                    this->clients_[slot] = -1;
                    this->client_threads_.erase(slot);
                });

                respond(new_socket);
            });
        }
    }

    L_DEBUG("DebugService: Thread finished");
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
        listen_fd_ = socket(p->ai_family, p->ai_socktype, IPPROTO_TCP);
        if(listen_fd_ == -1) continue;
        if(bind(listen_fd_, p->ai_addr, p->ai_addrlen) == 0) break;
    }

    if(!p) {
        L_ERROR("DebugService: Error binding to socket");
        return false;
    }

    freeaddrinfo(res);

    if(listen(listen_fd_, 10) != 0) {
        L_ERROR("DebugService: listen() error");
        return false;
    }

    return true;
}

void DebugService::respond(int client_socket) {
    L_DEBUG("DebugService: Handling request");

    const int MAX_LENGTH = 1024 * 1024;
    char msg[MAX_LENGTH] = {0};

    int rcvd = recv(client_socket, msg, MAX_LENGTH, 0);
    if(rcvd < 0) {
        L_WARN("DebugService: Error receiving from client");
    } else if(rcvd == 0) {
        L_WARN("DebugService: Client disconnected");
    } else {
        // Handle message
    }
}

}
