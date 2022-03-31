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


template<typename Func, typename T>
struct CallAndSetState {
public:
    void operator()(Func f, typename PromiseState<T>::ptr state) {
        state->value = f();
    }
};

template<typename Func>
struct CallAndSetState<Func, void> {
public:
    void operator()(Func f, typename PromiseState<void>::ptr state) {
        f();
        state->value = true;
    }
};

}

template<typename T>
class Promise {
public:
    bool is_ready() const {
        return state_ && state_->value;
    }

    T& value() const {
        assert(state_->value);
        return (state_->value.value());
    }

    /* Starts another coroutine that waits until
     * this one has been fulfilled */
    template<typename Func>
    Promise<typename std::result_of<Func()>::type> then(Func func);

    /* Default constructor does nothing */
    Promise() = default;

    bool is_initialized() const {
        return bool(state_);
    }

private:
    template<typename Func>
    friend Promise<typename std::result_of<Func()>::type> cr_async(Func func);

    Promise(typename promise_impl::PromiseState<T>::ptr state):
        state_(state) {

    }

    typename promise_impl::PromiseState<T>::ptr state_;
};

template<>
class Promise<void> {
private:
    template<typename Func>
    friend Promise<typename std::result_of<Func()>::type> cr_async(Func func);

    Promise(typename promise_impl::PromiseState<void>::ptr state):
        state_(state) {

    }

    typename promise_impl::PromiseState<void>::ptr state_;

    /* Default constructor does nothing */
    Promise() = default;

    bool is_initialized() const {
        return bool(state_);
    }

public:
    bool is_ready() const {
        return state_ && state_->value;
    }

    void value() const {}

    /* Starts another coroutine that waits until
     * this one has been fulfilled */
    template<typename Func>
    Promise<typename std::result_of<Func()>::type> then(Func func);
};

void _trigger_coroutine(std::function<void ()> func);
void _trigger_idle_updates();

template<typename Func>
Promise<typename std::result_of<Func()>::type> cr_async(Func func) {
    typedef typename std::result_of<Func()>::type T;

    auto state = std::make_shared<typename promise_impl::PromiseState<T>>();
    Promise<T> promise(state);

    _trigger_coroutine([state, func]() {
        promise_impl::CallAndSetState<Func, T>()(func, state);
    });

    return promise;
}

void cr_yield();


/* Starts another coroutine that waits until
 * this one has been fulfilled */

template<typename T>
template<typename Func>
Promise<typename std::result_of<Func()>::type> Promise<T>::then(Func func) {

    /* Copy the smart pointer and pass to the callback */
    auto state = state_;

    auto cb = [this, func, state]() -> typename std::result_of<Func()>::type {
        while(!state->value) {
            cr_yield();
        }

        return func(this->value());
    };

    return cr_async(cb);
}

template<typename Func>
Promise<typename std::result_of<Func()>::type> Promise<void>::then(Func func) {
    auto state = state_;

    auto cb = [this, func, state]() -> typename std::result_of<Func()>::type {
        while(!state->value) {
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
T cr_await(const Promise<T>& promise) {
    while(!promise.is_ready()) {
        if(cort::within_coroutine()){
            cr_yield();
        } else {
            _trigger_idle_updates();
            thread::sleep(0);
        }
    }

    return promise.value();
}

}
