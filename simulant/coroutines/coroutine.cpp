#include <unordered_map>
#include <utility>
#include "coroutine.h"
#include "../threads/thread.h"
#include "../threads/mutex.h"
#include "../threads/condition.h"
#include "../application.h"
#include "../time_keeper.h"

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

    /* If non-zero then the coroutine won't resume until this time
     * has passed */
    uint64_t resume = 0;

    Context* next = nullptr;
    Context* prev = nullptr;
};

static Context* CONTEXTS = nullptr;
static CoroutineID ID_COUNTER = 0;

#if defined(__PSP__) || defined(__DREAMCAST__)

static thread::Mutex CURRENT_CONTEXT_MUTEX;
static std::unordered_map<thread::ThreadID, Context*> THREAD_CONTEXTS;

static Context* current_context() {
    thread::Lock<thread::Mutex> l(CURRENT_CONTEXT_MUTEX);
    auto it = THREAD_CONTEXTS.find(thread::this_thread_id());
    if(it == THREAD_CONTEXTS.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

static void set_current_context(Context* context) {
    thread::Lock<thread::Mutex> l(CURRENT_CONTEXT_MUTEX);
    THREAD_CONTEXTS[thread::this_thread_id()] = context;
}

#else

static thread_local Context* CURRENT_CONTEXT = nullptr;

static Context* current_context() {
    return CURRENT_CONTEXT;
}

static void set_current_context(Context* context) {
    CURRENT_CONTEXT = context;
}

#endif

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
    set_current_context(context);

    context->func();

    context->mutex.lock();
    context->is_running = false;
    context->is_finished = true;
    context->cond.notify_one();
    context->mutex.unlock();
}

COResult resume_coroutine(CoroutineID id) {
    assert(!current_context());

    auto routine = find_coroutine(id);
    if(!routine) {
        return CO_RESULT_INVALID;
    }

    routine->mutex.lock();

    /* We've finished, do nothing */
    if(routine->is_finished) {
        routine->mutex.unlock();
        return CO_RESULT_FINISHED;
    }

    /* Don't resume the coroutine if we're not ready yet */
    if(routine->resume) {
        auto now = get_app()->time_keeper->now_in_us();
        if(routine->resume > now) {
            routine->mutex.unlock();
            return CO_RESULT_RUNNING;
        } else {
            /* Reset, we can run now */
            routine->resume = 0;
        }
    }

    routine->is_running = true;
    if(!routine->is_started) {
        /* Start the coroutine running */
        routine->thread = new thread::Thread(&run_coroutine, routine);
        routine->is_started = true;
        while(routine->is_running) {
            routine->cond.wait(routine->mutex);
        }
        routine->mutex.unlock();
    } else {
        /* Tell the coroutine to run */
        routine->cond.notify_one();
        routine->mutex.unlock();

        routine->mutex.lock();
        while(routine->is_running) {
            routine->cond.wait(routine->mutex);
        }
        routine->mutex.unlock();
    }

    return (routine->is_finished) ? CO_RESULT_FINISHED : CO_RESULT_RUNNING;
}

void yield_coroutine(const smlt::Seconds& from_now) {
    if(!current_context()) {
        /* Yield called from outside a coroutine
         * just return */
        return;
    }

    auto current = current_context();

    current->mutex.lock();
    current->is_running = false;

    /* Set the timeout if necessary */
    if(from_now > 0.0f) {
        current->resume = get_app()->time_keeper->now_in_us() + (
            from_now.to_float() * 1000000.0f
        );
    }

    current->cond.notify_one();

    while(!current->is_running) {
        current->cond.wait(current->mutex);
    }

    if(current->is_terminating) {
        /* This forces an incomplete coroutine to
         * end if stop_coroutine has been called */
        current->is_running = false;
        current->is_finished = true;
        current->cond.notify_one();
        current->mutex.unlock();
        thread::Thread::exit();
    }

    current->mutex.unlock();
}

bool within_coroutine() {
    return bool(current_context());
}

void stop_coroutine(CoroutineID id) {
    assert(!current_context());

    auto routine = find_coroutine(id);

    if(routine) {
        auto& context = *routine;
        if(context.is_started) {
            context.mutex.lock();
            context.is_terminating = true;
            context.mutex.unlock();

            /* This will cause the thread to terminate now */
            while(resume_coroutine(id) != CO_RESULT_FINISHED) {}

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
