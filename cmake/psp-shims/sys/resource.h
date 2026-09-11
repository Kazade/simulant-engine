/* PSPSDK's real <sys/resource.h> (see /usr/local/pspdev/psp/include/sys/
 * resource.h) only implements getrusage()/RUSAGE_* (resource *usage*
 * reporting) -- it has no setrlimit()/struct rlimit/RLIMIT_* at all
 * (resource *limits*, a concept that doesn't really apply to a single-
 * process console OS). glib/gtestutils.c's g_test_disable_crash_reporting()
 * still calls setrlimit(RLIMIT_CORE, ...) unconditionally once
 * HAVE_SYS_RESOURCE_H is set (which it is, since the header does exist),
 * with no separate "and setrlimit() actually exists" check.
 *
 * #include_next pulls in PSPSDK's real header for the getrusage() bits
 * (found via the *rest* of the include path, after this directory --
 * see cmake/GLibCrossFilePSP.cmake, which puts this shim dir first) and
 * this adds the missing pieces as a harmless no-op: succeeding without
 * doing anything is exactly what glib wants here (it's disabling a
 * limit that doesn't exist on this platform to begin with).
 */
#ifndef SIMULANT_PSP_SYS_RESOURCE_H
#define SIMULANT_PSP_SYS_RESOURCE_H

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
