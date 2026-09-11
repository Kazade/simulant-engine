/* KOS implements sched_yield() -- the symbol is right there in the
 * libraries the link already pulls in -- but newlib's <sched.h> only
 * *declares* it when the C library advertises POSIX threads:
 *
 *     #if defined(_POSIX_THREADS) || defined(_POSIX_PRIORITY_SCHEDULING)
 *     int sched_yield( void );
 *     #endif
 *
 * and KOS defines neither of those feature macros, even though it does
 * provide pthreads. So glib/gthread-posix.c's g_thread_yield_impl() calls
 * sched_yield() and fails to compile with an implicit declaration, despite
 * the function existing and linking fine.
 *
 * Declaring it here rather than defining _POSIX_THREADS project-wide keeps
 * the change to exactly the one function that's actually missing: turning
 * that feature macro on would also unhide a pile of other POSIX
 * declarations whose implementations KOS may not have, trading a clear
 * compile error for link errors later.
 */
#ifndef SIMULANT_DREAMCAST_SCHED_H
#define SIMULANT_DREAMCAST_SCHED_H

#include_next <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

int sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif
