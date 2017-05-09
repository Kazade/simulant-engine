#pragma once


/*
 * Because of the Dreamcast's lack of atomics the GCC
 * version of the this_thread namespace doesn't exist so we
 * simulate it here using nanosleep etc.
 */

#include <time.h>
#include <chrono>

namespace stdX {

namespace this_thread {

template< class Rep, class Period >
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
    struct timespec tv;
    struct timespec rem;

    auto usec = std::chrono::duration_cast<std::chrono::nanoseconds>(sleep_duration);
    if(usec <= std::chrono::nanoseconds(0)) {
        tv.tv_sec = tv.tv_nsec = 0;
    } else {
        tv.tv_sec = usec.count() / 1000000000;
        tv.tv_nsec = usec.count() % 1000000000;
    }

    nanosleep(&tv, &rem);
}

}

}
