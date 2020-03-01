#include <map>
#include <utility>
#include "coroutines.h"
#include "threads/thread.h"
#include "threads/mutex.h"
#include "threads/condition.h"

namespace smlt {

struct Context {
    coroutine_id id;
    bool is_running = false;
    bool is_started = false;
    bool is_finished = false;
    bool is_terminating = false;
    thread::Thread* thread = nullptr;
    std::function<void ()> func;

    thread::Mutex mutex;
    thread::Condition cond;
};

static std::map<coroutine_id, Context> CONTEXTS;
static coroutine_id ID_COUNTER = 0;

thread_local static Context* CURRENT_CONTEXT = nullptr;

coroutine_id start_coroutine(std::function<void ()> f) {
    auto& new_context = CONTEXTS[++ID_COUNTER];
    new_context.id = ID_COUNTER;
    new_context.func = f;
    return new_context.id;
}

static void run_coroutine(Context* context) {
    CURRENT_CONTEXT = context;

    context->mutex.lock();
    context->func();
    context->is_running = false;
    context->is_finished = true;
    context->cond.notify_one();
    context->mutex.unlock();
}

COResult resume_coroutine(coroutine_id id) {
    assert(!CURRENT_CONTEXT);

    auto it = CONTEXTS.find(id);
    if(it == CONTEXTS.end()) {
        return CO_RESULT_INVALID;
    }

    if(it->second.is_finished) {
        return CO_RESULT_FINISHED;
    }

    auto& context = it->second;
    context.mutex.lock();
    if(!it->second.is_started) {
        context.thread = new thread::Thread(&run_coroutine, &context);
        context.is_started = true;
    } else {
        context.is_running = true;
        context.cond.notify_one();
    }
    context.mutex.unlock();

    return CO_RESULT_RUNNING;
}

void yield_coroutine() {
    CURRENT_CONTEXT->is_running = false;
    CURRENT_CONTEXT->cond.notify_one();
    while(!CURRENT_CONTEXT->is_running) {
        CURRENT_CONTEXT->cond.wait(CURRENT_CONTEXT->mutex);
        if(CURRENT_CONTEXT->is_terminating) {
            thread::Thread::exit();
        }
    }
}

void stop_coroutine(coroutine_id id) {
    assert(!CURRENT_CONTEXT);

    auto it = CONTEXTS.find(id);
    if(it != CONTEXTS.end()) {
        auto& context = it->second;
        if(context.is_started) {
            context.mutex.lock();
            context.is_terminating = true;
            context.cond.notify_one();
            context.mutex.unlock();

            /* This will cause the thread to terminate now */
            resume_coroutine(id);

            context.thread->join();

            delete context.thread;
            context.thread = nullptr;
        }

        CONTEXTS.erase(it);
    }
}

}
