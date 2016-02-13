#ifndef DEBUG_SERVICE_H
#define DEBUG_SERVICE_H

#include <future>

namespace kglt {

class WindowBase;

/*
 * The Debug service is a simple JSON based API
 * for inspecting the state of the engine for debugging
 */
class DebugService {
public:
    const static int PORT = 112358;
    const static int MAX_CLIENTS = 1;

    DebugService(WindowBase* window);

    void start();
    void stop();

private:
    std::string command_screens(); // Screen info
    std::string command_stages(); // Stage info including actors etc.
    std::string command_pipelines(); // Details on the render sequence
    std::string command_rendering(); // Details on the render sequence

    int clients_[MAX_CLIENTS];
    int listen_fd_;

    bool start_server();
    void respond(int client_id);

    bool running_ = false;
    std::future<void> handle_;

    void run();
};

}

#endif // DEBUG_SERVICE_H
