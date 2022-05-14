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
    if(cort::within_coroutine()) {
        cort::yield_coroutine();
    } else {
        /* Main thread, just update the coroutines */
        get_app()->update_coroutines();
    }
}

void cr_yield_and_wait(const smlt::Seconds& seconds) {
    cort::yield_coroutine(seconds);
}

void _trigger_idle_updates() {
    get_app()->update_coroutines();
}

}
