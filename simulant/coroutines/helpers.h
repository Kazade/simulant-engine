#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "../generic/static_if.h"
#include "coroutine.h"
#include "../threads/thread.h"

namespace smlt {

namespace promise_impl {

template<typename T>
struct PromiseState {
    typedef std::shared_ptr<PromiseState<T>> ptr;

    optional<T> value;
};

template<>
struct PromiseState<void> {
    typedef std::shared_ptr<PromiseState<void>> ptr;
    bool value;
};

template<typename T, typename Func>
void call_func_set_state(Func f, typename PromiseState<T>::ptr state) {
    state->value = f();
}

template<typename Func>
void call_func_set_state(Func f, typename PromiseState<void>::ptr state) {
    f();
    state->value = true;
}

}

template<typename T>
class CRPromise {
public:
    bool is_ready() const {
        return state_ && state_->value;
    }

    T& value() const {
        return (state_->value);
    }

    /* Starts another coroutine that waits until
     * this one has been fulfilled */
    template<typename Func>
    CRPromise<typename std::result_of<Func()>::type> then(Func func);

private:
    template<typename Func>
    friend CRPromise<typename std::result_of<Func()>::type> cr_async(Func func);

    CRPromise(typename promise_impl::PromiseState<T>::ptr state):
        state_(state) {

    }

    typename promise_impl::PromiseState<T>::ptr state_;
};

template<>
class CRPromise<void> {
private:
    template<typename Func>
    friend CRPromise<typename std::result_of<Func()>::type> cr_async(Func func);

    CRPromise(typename promise_impl::PromiseState<void>::ptr state):
        state_(state) {

    }

    typename promise_impl::PromiseState<void>::ptr state_;

public:
    bool is_ready() const {
        return state_ && state_->value;
    }

    void value() const {}

    /* Starts another coroutine that waits until
     * this one has been fulfilled */
    template<typename Func>
    CRPromise<typename std::result_of<Func()>::type> then(Func func);
};

void _trigger_coroutine(std::function<void ()> func);
void _trigger_idle_updates();

template<typename Func>
CRPromise<typename std::result_of<Func()>::type> cr_async(Func func) {
    typedef typename std::result_of<Func()>::type T;

    auto state = std::make_shared<typename promise_impl::PromiseState<T>>();
    CRPromise<T> promise(state);

    _trigger_coroutine([state, func]() {
        promise_impl::call_func_set_state(func, state);
    });

    return promise;
}

void cr_yield();


/* Starts another coroutine that waits until
 * this one has been fulfilled */

template<typename T>
template<typename Func>
CRPromise<typename std::result_of<Func()>::type> CRPromise<T>::then(Func func) {
    auto cb = [this, func]() -> typename std::result_of<Func()>::type {
        while(!state_->value) {
            cr_yield();
        }

        return func();
    };

    return cr_async(cb);
}

template<typename Func>
CRPromise<typename std::result_of<Func()>::type> CRPromise<void>::then(Func func) {
    auto cb = [this, func]() -> typename std::result_of<Func()>::type {
        while(!state_->value) {
            cr_yield();
        }

        return func();
    };

    return cr_async(cb);
}

/* Will wait for the promise returned by a coroutine
 * to be fulfilled. Be careful when calling from the main
 * thread, because in this case only idle tasks and
 * coroutines will be updated (no rendering or scene
 * updates will happen while you are waiting).
 *
 * In the main thread you may prefer to use
 * start_coroutine(...).then(...)
 */
template<typename T>
T& cr_await(const CRPromise<T>& promise) {
    while(!promise.is_ready()) {
        if(cort::within_coroutine()){
            cr_yield();
        } else {
            _trigger_idle_updates();
            thread::sleep(0);
        }
    }

    return promise->value();
}

}
