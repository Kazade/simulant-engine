#include <map>
#include <utility>
#include "coroutine.h"
#include "../threads/thread.h"
#include "../threads/mutex.h"
#include "../threads/condition.h"

namespace smlt {
namespace cort {

struct Context {
    CoroutineID id;
    bool is_running = false;
    bool is_started = false;
    bool is_finished = false;
    bool is_terminating = false;
    thread::Thread* thread = nullptr;
    std::function<void ()> func;

    thread::Mutex mutex;
    thread::Condition cond;

    Context* next = nullptr;
    Context* prev = nullptr;
};

static Context* CONTEXTS = nullptr;
static CoroutineID ID_COUNTER = 0;

/* GCC < 4.8 doesn't have thread_local so we use the __thread
 * extension instead */
#ifdef __GNUC__
#if __GNUC_MAJOR__ < 5
    #define thread_local __thread
#endif
#endif

static thread_local Context* CURRENT_CONTEXT = nullptr;

CoroutineID start_coroutine(std::function<void ()> f) {
    if(!CONTEXTS) {
        CONTEXTS = new Context();
        CONTEXTS->id = ++ID_COUNTER;
        CONTEXTS->func = f;
    } else {
        auto prev_root = CONTEXTS;
        CONTEXTS = new Context();
        CONTEXTS->next = prev_root;

        if(prev_root) {
            prev_root->prev = CONTEXTS;
        }

        CONTEXTS->id = ++ID_COUNTER;
        CONTEXTS->func = f;
    }

    return CONTEXTS->id;
}

static Context* find_coroutine(CoroutineID id) {
    for(auto c = CONTEXTS; c; c = c->next) {
        if(c->id == id) {
            return c;
        }
    }

    return nullptr;
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

COResult resume_coroutine(CoroutineID id) {
    assert(!CURRENT_CONTEXT);

    auto routine = find_coroutine(id);

    if(!routine) {
        return CO_RESULT_INVALID;
    }

    /* We've finished, do nothing */
    if(routine->is_finished) {
        return CO_RESULT_FINISHED;
    }

    auto& context = *routine;
    context.mutex.lock();
    context.is_running = true;
    if(!context.is_started) {
        /* Start the coroutine running */
        context.thread = new thread::Thread(&run_coroutine, &context);
        context.is_started = true;
        context.mutex.unlock();
    } else {
        /* Tell the coroutine to run */
        context.mutex.unlock();
        context.cond.notify_one();
    }

    context.mutex.lock();
    /* Wait for the coroutine to yield */
    while(context.is_running) {
        context.cond.wait(context.mutex);
    }

    context.mutex.unlock();

    return CO_RESULT_RUNNING;
}

void yield_coroutine() {
    if(!CURRENT_CONTEXT) {
        /* Yield called from outside a coroutine
         * just return */
        return;
    }

    CURRENT_CONTEXT->is_running = false;
    CURRENT_CONTEXT->cond.notify_one();
    while(!CURRENT_CONTEXT->is_running) {
        CURRENT_CONTEXT->cond.wait(CURRENT_CONTEXT->mutex);
        if(CURRENT_CONTEXT->is_terminating) {
            /* This forces an incomplete coroutine to
             * end if stop_coroutine has been called */
            thread::Thread::exit();
        }
    }
}

bool within_coroutine() {
    return bool(CURRENT_CONTEXT);
}

void stop_coroutine(CoroutineID id) {
    assert(!CURRENT_CONTEXT);

    auto routine = find_coroutine(id);

    if(routine) {
        auto& context = *routine;
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

        if(CONTEXTS == routine) {
            CONTEXTS = routine->next;
        }

        if(routine->next) routine->next->prev = routine->prev;
        if(routine->prev) routine->prev->next = routine->next;

        delete routine;
    }
}

}
}
