#include "helpers.h"
#include "../window.h"

namespace smlt {

void start_coroutine(std::function<void ()> func) {
    Application* app = get_app();

    if(app) {
        auto window = app->window.get();
        window->start_coroutine(func);
    }
}

void yield_coroutine() {
    cort::yield_coroutine();
}

}
