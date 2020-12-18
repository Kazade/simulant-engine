#pragma once

#include <functional>

#include "coroutine.h"
#include "../application.h"

namespace smlt {

void start_coroutine(std::function<void ()> func);
void yield_coroutine();
}
