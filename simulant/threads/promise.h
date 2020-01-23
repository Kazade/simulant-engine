#pragma once

#include "../generic/optional.h"
#include "mutex.h"

namespace smlt {
namespace thread {

template<typename T>
class Promise {
private:
    class PromiseData {
        Mutex mutex_;
        smlt::optional<T> value_;
        bool failed_ = false;

    public:
        void set_failed() {
            Lock<Mutex> guard(mutex_);
            failed_ = true;
        }

        void set_value(T&& v) {
            Lock<Mutex> guard(mutex_);
            value_ = optional<T>(std::move(v));
        }
    };

public:
    typedef T result_type;

    bool is_ready() const {
        Lock<Mutex> guard(data_->mutex_);
        return data_->value_.has_value();
    }

    bool is_failed() const {
        Lock<Mutex> guard(data_->mutex_);
        return data_->failed_;
    }

    T value() const {
        Lock<Mutex> guard(data_->mutex_);
        return data_->value_.value();
    }

private:
    template<typename F, typename... Args>
    friend Promise<T> async(F&&, Args&&...);

    Promise(std::shared_ptr<PromiseData> data):
        data_(data) {

        assert(data_);
    }

    std::shared_ptr<PromiseData> data_;
};

}
}
