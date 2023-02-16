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
        auto app = get_app();
        if(thread::this_thread_id() == app->thread_id()) {
            /* Main thread, just update the coroutines */
            get_app()->update_coroutines();
        }
    }
}

void cr_yield_for(const smlt::Seconds& seconds) {
    cort::yield_coroutine(seconds);
}

void cr_run_main(std::function<void ()> func) {
    if(cort::within_coroutine()) {
        get_app()->cr_synced_function_ = func;
        cr_yield();
    } else {
        /* Already in the main thread */
        func();
    }
}

void _trigger_idle_updates() {
    get_app()->update_coroutines();
}

}
