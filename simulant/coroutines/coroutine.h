#pragma once

#include <functional>

namespace smlt {

typedef uint32_t CoroutineID;

enum COResult {
    CO_RESULT_RUNNING = 0,
    CO_RESULT_FINISHED,
    CO_RESULT_INVALID
};

CoroutineID start_coroutine(std::function<void ()> f);
void stop_coroutine(CoroutineID id);
COResult resume_coroutine(CoroutineID id);
void yield_coroutine();
bool within_coroutine();

}
