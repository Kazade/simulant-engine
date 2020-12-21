#include "helpers.h"
#include "../window.h"
#include "../application.h"

namespace smlt {

void _trigger_coroutine(std::function<void ()> func) {
    Application* app = get_app();

    if(app) {
        auto window = app->window.get();
        window->start_coroutine(func);
    }
}

void yield_coroutine() {
    cort::yield_coroutine();
}

void _trigger_idle_updates() {
    Window* window = get_app()->window.get();
    window->update_idle_tasks_and_coroutines();
}

}
