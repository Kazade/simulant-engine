#ifndef DEBUG_SERVICE_H
#define DEBUG_SERVICE_H

#include <future>
#include <map>

namespace kglt {

class WindowBase;

/*
 * The Debug service is a simple JSON based API
 * for inspecting the state of the engine for debugging
 */
class DebugService {
public:
    const static int PORT = 11235;
    const static int MAX_CLIENTS = 1;

    DebugService(WindowBase* window):
        window_(window) {}

    ~DebugService();

    void start();
    void stop();

private:
    WindowBase* window_;

    std::string command_screens(); // Screen info
    std::string command_stages(); // Stage info including actors etc.
    std::string command_pipelines(); // Details on the render sequence
    std::string command_rendering(); // Details on the render sequence

    int clients_[MAX_CLIENTS];
    int listen_fd_;

    bool start_server();
    void respond(int client_id);

    volatile bool running_ = false;

    std::future<void> handle_;
    std::map<int, std::future<void>> client_threads_;

    std::mutex client_lock_;

    void run();
};

}

#endif // DEBUG_SERVICE_H
