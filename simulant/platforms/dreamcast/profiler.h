#pragma once

#include <stdbool.h>

/*
 * The Dreamcast doesn't have any kind of profiling support from GCC
 * so this is a sampling profiler driven by a TMU1 timer interrupt.
 *
 * profiler_reset() discards every sample collected so far without writing
 * gmon.out. Call it once the app has finished loading/initialising so the
 * resulting profile covers only the steady-state window you care about.
 */
#ifdef __cplusplus
extern "C" {
#endif

void profiler_init(const char* output);
void profiler_start();
void profiler_reset();
bool profiler_stop();
void profiler_clean_up();

#ifdef __cplusplus
}
#endif
