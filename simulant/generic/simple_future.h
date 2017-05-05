#pragma once

#include <type_traits>
#include <thread>
#include <mutex>
#include "exception_ptr_lite.hpp"
#include "static_if.h"
#include "simple_this_thread.h"

/*
 * The Dreamcast doesn't support <future> so this is a little wrapper around std::thread which simulates it really crappily
 * with promise, or packaged_task or anything useful and - this is probably broken in many many ways!
 */


namespace stdX {

enum class future_status {
    ready,
    timeout
};

template<typename T>
class future {
public:
    future() {}
    future(future&& other);
    future(const future& other) = delete;

    future& operator=(future&& other) {
        state_ = std::move(other.state_);
        return *this;
    }

    future& operator=(const future& other) = delete;

    T get() {
        wait();

        std::lock_guard<std::mutex> lock(state_->lock_);
        if(state_->exception_) {
            stdX::rethrow_exception(state_->exception_);
        } else {
            return state_->result_;
        }
    }

    template< class Rep, class Period >
    future_status wait_for(const std::chrono::duration<Rep,Period>& timeout_duration) const {
        {
            std::lock_guard<std::mutex> lock(state_->lock_);
            if(state_->status_ == future_status::ready) {
                return future_status::ready;
            }

        }
        auto deadline = std::chrono::system_clock::now() + timeout_duration;

        while(deadline > std::chrono::system_clock::now()) {
            this_thread::sleep_for(std::chrono::microseconds(0));
            std::lock_guard<std::mutex> lock(state_->lock_);
            if(state_->status_ == future_status::ready) {
                return future_status::ready;
            }
        }

        std::lock_guard<std::mutex> lock(state_->lock_);
        return state_->status_;
    }

    void wait() {
        while(wait_for(std::chrono::microseconds(0)) != future_status::ready) {
            this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }

    bool valid() const {
        return bool(state_);
    }

    struct future_state {
        std::mutex lock_;

        T result_;
        future_status status_ = future_status::timeout;
        stdX::exception_ptr exception_;
    };

    future(std::shared_ptr<future_state> state):
        state_(state) {

    }

private:
    std::shared_ptr<future_state> state_;
};


template<>
class future<void> {
public:
    future() {}
    future(future&& other);
    future(const future& other) = delete;

    future& operator=(future&& other) {
        state_ = std::move(other.state_);
        return *this;
    }

    future& operator=(const future& other) = delete;

    template< class Rep, class Period >
    future_status wait_for(const std::chrono::duration<Rep,Period>& timeout_duration) const {
        {
            std::lock_guard<std::mutex> lock(state_->lock_);
            if(state_->status_ == future_status::ready) {
                return future_status::ready;
            }

        }
        auto deadline = std::chrono::system_clock::now() + timeout_duration;

        while(deadline > std::chrono::system_clock::now()) {
            this_thread::sleep_for(std::chrono::microseconds(0));
            std::lock_guard<std::mutex> lock(state_->lock_);
            if(state_->status_ == future_status::ready) {
                return future_status::ready;
            }
        }

        std::lock_guard<std::mutex> lock(state_->lock_);
        return state_->status_;
    }

    void wait() {
        while(wait_for(std::chrono::microseconds(0)) != future_status::ready) {
            this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }

    void get() {
        wait();

        std::lock_guard<std::mutex> lock(state_->lock_);
        if(state_->exception_) {
            stdX::rethrow_exception(state_->exception_);
        }
    }

    struct future_state {
        std::mutex lock_;
        future_status status_ = future_status::timeout;
        stdX::exception_ptr exception_;
    };

    future(std::shared_ptr<future_state> state):
        state_(state) {

    }
private:
    std::shared_ptr<future_state> state_;
};


template< class T >
using decay_t = typename std::decay<T>::type;

template< class T >
using result_of_t = typename std::result_of<T>::type;

template< bool B, class T = void >
using enable_if_t = typename std::enable_if<B,T>::type;


namespace simple_future_detail {

template<typename ResultType, typename Function, typename... Args, enable_if_t<std::is_void<ResultType>::value, int> = 42>
void call_func(typename future<ResultType>::future_state& state, Function&& f, Args&&... args) {
    f(std::forward<Args>(args)...);
    std::lock_guard<std::mutex> lock(state.lock_);
    state.status_ = future_status::ready;
}

template<typename ResultType, typename Function, typename... Args, enable_if_t<!std::is_void<ResultType>::value, int> = 42>
void call_func(typename future<ResultType>::future_state& state, Function&& f, Args&&... args) {
    ResultType ret = f(std::forward<Args>(args)...);
    std::lock_guard<std::mutex> lock(state.lock_);
    state.result_ = ret;
    state.status_ = future_status::ready;
}

}


template<typename Function, typename ...Args>
future<result_of_t<decay_t<Function>(decay_t<Args>...)>> async(Function&& f, Args&&... args) {
    typedef result_of_t<decay_t<Function>(decay_t<Args>...)> ResultType;

    auto state = std::make_shared<typename future<ResultType>::future_state>();

    auto processor = [state](Function&& f, Args&&... args) {
        try {
            simple_future_detail::call_func<ResultType, Function, Args...>(*state, std::forward<Function>(f), std::forward<Args>(args)...);
        } catch (std::exception& e) {
            std::lock_guard<std::mutex> lock(state->lock_);
            state->exception_ = stdX::current_exception();
            state->status_ = future_status::ready;
        }
    };

    std::thread t(processor, f, std::forward<Args>(args)...);
    t.detach();

    return future<ResultType>(state);
}

}
