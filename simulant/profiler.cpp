#ifdef _arch_dreamcast
#include <kos.h>
#endif

#include <iostream>
#include <iomanip>
#include <chrono>

#include "profiler.h"

namespace smlt {

RootProfiler* Profiler::root_ = nullptr;

Profiler::Profiler(const std::string& name):
    name_(name) {

    init_root();

    root_->stack_.push_back(this);

    start_time_ = current_time_in_us();
}

void Profiler::init_root() {
    if(!Profiler::root_) {
        Profiler::root_ = new RootProfiler();
    }
}

uint64_t Profiler::current_time_in_us() const {
#ifdef _arch_dreamcast
    return timer_us_gettime64();
#else
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
#endif
}

std::string RootProfiler::generate_current_path() {
    std::string path;

    uint32_t i = 0;
    uint32_t c = stack_.size();

    for(auto& profiler: stack_) {
        path += profiler->name();

        if(i < c - 1) {
            path += ".";
        }
    }

    return path;
}

void RootProfiler::print_stats() {
    std::cout << std::setiosflags(std::ios::fixed)
              << std::setprecision(3)
              << std::setw(60)
              << std::left
              << "Path"
              << std::setw(30)
              << std::left
              << "Average"
              << "Total"
              << std::endl;

    for(auto& value: results_) {
        std::string path = value.first;
        ProfilerResult result = value.second;

        float ms = float(result.total_us) / 1000;
        float avg = ms / float(result.total_calls);

        std::cout << std::setiosflags(std::ios::fixed)
                  << std::setprecision(3)
                  << std::setw(60)
                  << std::left
                  << path
                  << std::setw(30)
                  << std::left
                  << avg
                  << ms
                  << "ms"
                  << std::endl;
    }
}

void Profiler::checkpoint(const std::string& name) {
    std::string path = root_->generate_current_path();
    std::string full_path = path + ":" + name;

    auto now = current_time_in_us();
    auto elapsed = now - start_time_;
    start_time_ = now;

    auto it = root_->results_.find(full_path);
    if(it == root_->results_.end()) {
        ProfilerResult result;
        result.total_us = elapsed;
        result.total_calls = 1;
        root_->results_[full_path] = result;
    } else {
        ProfilerResult& result = (*it).second;
        result.total_us += elapsed;
        result.total_calls++;
    }
}

Profiler::~Profiler() {
    root_->stack_.pop_back();
}

}
