#pragma once

#include "promise.h"
#include "thread.h"

namespace smlt {
namespace thread {

template<typename F>
auto async(F&& func) -> Promise<decltype(func())> {
    typedef decltype(func()) ReturnType;
    auto data = std::make_shared<typename Promise<ReturnType>::PromiseData>();

    Promise<ReturnType> promise(data);

    auto runner = [func, data]() {
        try {
            auto ret = func();
            data->set_value(std::move(ret));
        } catch(const std::exception& e) {
            std::cerr << "Exception in async function: " << e.what() << std::endl;
            data->set_failed();
        } catch(...) {
            std::cerr << "Critical exception in async function" << std::endl;
            data->set_failed();
        }
    };

    Thread(runner).detach();

    return promise;
}

template<typename F, typename... Args>
auto async(F&& func, Args&&... args) -> Promise<decltype(func(args...))>{
    typedef decltype(func(args...)) ReturnType;

    auto data = std::make_shared<typename Promise<ReturnType>::PromiseData>();
    Promise<ReturnType> promise(data);

    auto f = std::bind(std::forward<F>(func), std::forward<Args>(args)...);

    auto runner = [f, data]() {
        try {
            auto ret = f();
            data->set_value(std::move(ret));
        } catch(const std::exception& e) {
            std::cerr << "Exception in async function: " << e.what() << std::endl;
            data->set_failed();
        } catch(...) {
            std::cerr << "Critical exception in async function" << std::endl;
            data->set_failed();
        }
    };

    Thread(runner).detach();

    return promise;
}

}
}
