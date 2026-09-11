/* Works around a real inconsistency in KOS's own <machine/time.h>.
 *
 * That header forward-declares `struct timespec;` only under C11 or later:
 *
 *     #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)) || ...
 *     struct timespec;
 *     extern int timespec_get(struct timespec *ts, int base);
 *     #endif
 *
 * but declares the POSIX clock functions that also take one under a
 * completely separate condition:
 *
 *     #if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L)
 *     extern int clock_gettime(__clockid_t clock_id, struct timespec *ts);
 *
 * Compile as C99 with _GNU_SOURCE -- which is exactly what glib does
 * (-std=gnu99 -D_GNU_SOURCE) -- and the second block is active while the
 * first isn't. newlib's <time.h> pulls this file in before it defines
 * `struct timespec`, so with no file-scope declaration in sight the tag in
 * clock_gettime's parameter list becomes *prototype-scoped*: a distinct,
 * incomplete type that happens to share a name. Passing newlib's real
 * `struct timespec *` then fails with the confusing
 *
 *     error: passing argument 2 of 'clock_gettime' from incompatible
 *            pointer type
 *     note: expected 'struct timespec *' but argument is of type
 *           'struct timespec *'
 *
 * Hoisting the forward declaration to file scope before KOS's header is
 * read makes both blocks refer to the same tag. It's a declaration only --
 * no layout is assumed here, and newlib still provides the real definition
 * -- so this changes nothing beyond which type the prototypes name.
 *
 * Worth reporting upstream: the forward declaration wants to be
 * unconditional (or to match the POSIX condition), not gated on C11.
 */
#ifndef __SIMULANT_DREAMCAST_MACHINE_TIME_H__
#define __SIMULANT_DREAMCAST_MACHINE_TIME_H__

struct timespec;
struct tm;

#include_next <machine/time.h>

/* timegm() has the same problem from the other direction: KOS implements it
 * (the symbol links fine, and glib's meson probe finds it and sets
 * HAVE_TIMEGM), but the declaration sits behind
 *
 *     #if (__STDC_VERSION__ >= 202300L) || (__cplusplus > 202002L) || \
 *         defined(__KOS_LIBC)
 *
 * so at -std=gnu99 glib calls it with no prototype in scope and
 * gtimer.c's mktime_utc() fails on the implicit declaration.
 *
 * Redeclared here with exactly KOS's own signature (a duplicate, matching
 * declaration is fine if the C23 block is active too). Declaring just this
 * one function is deliberate -- defining __KOS_LIBC to open that block
 * wholesale would also unhide neighbours like timespec_getres() that may
 * not have implementations behind them.
 */
extern __time_t timegm(struct tm *timeptr);

#endif
