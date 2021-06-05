#pragma once

#include <cstdio>
#include <chrono>
#include <type_traits>
#include "thread.h"
#include "mutex.h"
#include "../logging.h"

namespace smlt {
namespace thread {

enum class FutureStatus {
    ready,
    timeout
};

class PromiseFailedError : public std::runtime_error {
public:
    PromiseFailedError():
        std::runtime_error("Async task failed") {}
};

template<typename T>
class Future {
public:
    struct FutureState {
        Mutex lock_;

        T result_;
        bool is_ready_ = false;
        bool is_failed_ = false;
    };

    Future(std::shared_ptr<FutureState> state):
        state_(state) {
    }

    Future() {}
    Future(Future&& other);
    Future(const Future& other) = delete;

    Future& operator=(Future&& other) {
        Lock<Mutex> guard(state_mutex_);
        state_ = std::move(other.state_);
        return *this;
    }

    Future& operator=(const Future& other) = delete;

    T get() {
        wait();

        T ret;

        {
            Lock<Mutex> lock(state_->lock_);
            if(state_->is_failed_) {
                throw PromiseFailedError();
            }

            assert(state_->is_ready_);
            ret = state_->result_;
        }

        Lock<Mutex> guard(state_mutex_);
        state_.reset();
        return ret;
    }

    template< class Rep, class Period >
    FutureStatus wait_for(const std::chrono::duration<Rep,Period>& timeout_duration) const {
        auto deadline = std::chrono::system_clock::now() + timeout_duration;

        while(std::chrono::system_clock::now() < deadline) {
            {
                Lock<Mutex> lock(state_->lock_);
                if(state_->is_ready_) {
                    return FutureStatus::ready;
                }
            }
            thread::yield();
        }

        Lock<Mutex> lock(state_->lock_);
        return (state_->is_ready_) ? FutureStatus::ready : FutureStatus::timeout;
    }

    void wait() {
        while(wait_for(std::chrono::microseconds(0)) != FutureStatus::ready) {}
    }

    bool is_valid() const {
        Lock<Mutex> guard(state_mutex_);
        return bool(state_);
    }

    bool is_ready() const {
        Lock<Mutex> lock(state_->lock_);
        return state_->is_ready_;
    }

    bool is_failed() const {
        Lock<Mutex> lock(state_->lock_);
        return state_->is_failed_;
    }

private:
    mutable Mutex state_mutex_;
    std::shared_ptr<FutureState> state_;
};


template<>
class Future<void> {
public:
    struct FutureState {
        Mutex lock_;
        bool is_ready_ = false;
        bool is_failed_ = false;
    };

    /* FIXME: Should be private */
    Future(std::shared_ptr<FutureState> state):
        state_(state) {
    }

    Future() {}
    Future(Future&& other);
    Future(const Future& other) = delete;

    Future& operator=(Future&& other) {
        Lock<Mutex> guard(state_mutex_);
        state_ = std::move(other.state_);
        return *this;
    }

    Future& operator=(const Future& other) = delete;

    template<class Rep, class Period>
    FutureStatus wait_for(const std::chrono::duration<Rep,Period>& timeout_duration) const {
        auto deadline = std::chrono::system_clock::now() + timeout_duration;

        while(deadline > std::chrono::system_clock::now()) {
            Lock<Mutex> lock(state_->lock_);
            if(state_->is_ready_) {
                return FutureStatus::ready;
            }
            sleep(0);
        }

        Lock<Mutex> lock(state_->lock_);
        return (state_->is_ready_) ? FutureStatus::ready : FutureStatus::timeout;
    }

    void wait() {
        while(wait_for(std::chrono::microseconds(0)) != FutureStatus::ready) {}
    }

    void get() {
        wait();

        Lock<Mutex> guard(state_mutex_);
        state_.reset();
    }

    bool is_valid() const {
        Lock<Mutex> guard(state_mutex_);
        return bool(state_);
    }

    bool is_ready() const {
        Lock<Mutex> lock(state_->lock_);
        return state_->is_ready_;
    }

    bool is_failed() const {
        Lock<Mutex> lock(state_->lock_);
        return state_->is_failed_;
    }

private:
    mutable Mutex state_mutex_;
    std::shared_ptr<FutureState> state_;

};


template< class T >
using decay_t = typename std::decay<T>::type;

template< class T >
using result_of_t = typename std::result_of<T>::type;

template< bool B, class T = void >
using enable_if_t = typename std::enable_if<B,T>::type;


namespace simple_Future_detail {

template<typename ResultType, typename Function, typename... Args, enable_if_t<std::is_void<ResultType>::value, int> = 42>
void call_func(std::shared_ptr<typename Future<ResultType>::FutureState> state, Function&& f, Args&&... args) {
    f(std::forward<Args>(args)...);
    Lock<Mutex> lock(state->lock_);
    state->is_ready_ = true;
}

template<typename ResultType, typename Function, typename... Args, enable_if_t<!std::is_void<ResultType>::value, int> = 42>
void call_func(std::shared_ptr<typename Future<ResultType>::FutureState> state, Function&& f, Args&&... args) {
    ResultType ret = f(std::forward<Args>(args)...);
    Lock<Mutex> lock(state->lock_);
    state->result_ = ret;
    state->is_ready_ = true;
}

}

template<typename ResultType, typename Function, typename ...Args>
void processor(std::shared_ptr<typename Future<ResultType>::FutureState> state, Function&& f, Args&&... args) {
    try {
        simple_Future_detail::call_func<ResultType, Function, Args...>(
            state, std::forward<Function>(f), std::forward<Args>(args)...
        );
    } catch (std::exception& e) {
        Lock<Mutex> lock(state->lock_);
        fprintf(stderr, "%s", e.what());
        state->is_ready_ = true;
        state->is_failed_ = true;
    }
}

template<class Function, class ...Args>
Future<result_of_t<Function(Args...)>> async(Function&& f, Args&&... args) {
    typedef result_of_t<Function(Args...)> ResultType;
    typedef std::shared_ptr<typename Future<ResultType>::FutureState> StateType;
    auto state = std::make_shared<typename Future<ResultType>::FutureState>();

    typedef std::function<void (StateType, Function, Args...)> WrapperType;

    WrapperType callback = WrapperType(&processor<ResultType, Function, Args...>);

    Thread(
        callback,
        state,
        std::forward<Function>(f), std::forward<Args>(args)...
    ).detach();

    Future<ResultType> ret(state);
    return ret;
}

}
}

