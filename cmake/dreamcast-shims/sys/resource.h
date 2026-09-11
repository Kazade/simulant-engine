/* KOS's newlib ships a <sys/resource.h>, but it only covers resource
 * *usage* (getrusage()/RUSAGE_*) -- there's no setrlimit(), no
 * `struct rlimit` and no RLIMIT_* at all, resource *limits* not being a
 * meaningful concept on a single-process console OS.
 *
 * glib/gtestutils.c's g_test_disable_crash_reporting() still calls
 * setrlimit(RLIMIT_CORE, ...) unconditionally once HAVE_SYS_RESOURCE_H is
 * set, which it is here because the header does exist -- there's no
 * separate "...and setrlimit() is actually implemented" check.
 *
 * #include_next picks up the real header for the getrusage() side (found
 * further along the include path, since cmake/GLibCrossFileDreamcast.cmake
 * puts this directory first), and the rest is added here as a no-op.
 * Succeeding without doing anything is exactly right for this caller: it's
 * trying to disable core dumps, and this platform has none to begin with.
 *
 * Mirrors cmake/psp-shims/sys/resource.h, which exists for the same reason.
 */
#ifndef SIMULANT_DREAMCAST_SYS_RESOURCE_H
#define SIMULANT_DREAMCAST_SYS_RESOURCE_H

#include_next <sys/resource.h>

#define RLIMIT_CORE 4

struct rlimit {
    unsigned long rlim_cur;
    unsigned long rlim_max;
};

static __inline__ int setrlimit(int resource, const struct rlimit* rlp) {
    (void) resource;
    (void) rlp;
    return 0;
}

#endif
