#pragma once

#include <functional>

namespace smlt {

typedef uint32_t coroutine_id;

enum COResult {
    CO_RESULT_RUNNING = 0,
    CO_RESULT_FINISHED,
    CO_RESULT_INVALID
};

coroutine_id start_coroutine(std::function<void ()> f);
void stop_coroutine(coroutine_id id);
COResult resume_coroutine(coroutine_id id);
void yield_coroutine();
bool within_coroutine();

}
