#include "helpers.h"
#include "../application.h"

namespace smlt {

void _trigger_coroutine(std::function<void ()> func) {
    Application* app = get_app();

    if(app) {
        app->start_coroutine(func);
    }
}

void cr_yield() {
    cort::yield_coroutine();
}

void _trigger_idle_updates() {
    get_app()->update_idle_tasks_and_coroutines();
}

}
