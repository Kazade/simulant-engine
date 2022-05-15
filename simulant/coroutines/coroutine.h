#pragma once

#include <cstdint>
#include <functional>

#include "../generic/optional.h"
#include "../types.h"

namespace smlt {
namespace cort {


typedef int32_t CoroutineID;

enum COResult {
    CO_RESULT_RUNNING = 0,
    CO_RESULT_FINISHED,
    CO_RESULT_INVALID
};



CoroutineID start_coroutine(std::function<void ()> func);
void stop_coroutine(CoroutineID id);
COResult resume_coroutine(CoroutineID id);
void yield_coroutine(const smlt::Seconds& from_now=smlt::Seconds());
bool within_coroutine();

}
}
