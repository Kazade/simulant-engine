/* POSIX functions glib references that KOS doesn't provide.
 *
 * These only show up at the final executable link, not while compiling
 * glib: static libraries pull in whole object files, so one call anywhere
 * in gfileutils.c or gmain.c drags in every undefined symbol that object
 * mentions, whether or not this program ever reaches that code path.
 *
 * access() is implemented properly rather than stubbed -- it's reached
 * through g_file_test(), which Simulant's asset loading genuinely relies
 * on. The rest have no meaningful implementation on a single-user console
 * with no process model or signal delivery, and are given the same answers
 * a POSIX caller would get on a system where they trivially succeed.
 *
 * Mirrors cmake/psp-shims/psp_glib_runtime_stubs.c, which covers the same
 * ground for PSPSDK.
 */
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef F_OK
#define F_OK 0
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif

/* KOS fills in st_mode with real type and permission bits (see
 * kernel/fs/fs.c and kernel/libc/newlib/newlib_stat.c), so the requested
 * access can be checked against them rather than guessed at. There's only
 * one user on this machine, so the owner bits are the relevant ones. */
int access(const char* path, int mode) {
    struct stat st;

    if(!path) {
        errno = EFAULT;
        return -1;
    }

    if(stat(path, &st) != 0) {
        /* stat() has already set errno (ENOENT and friends). */
        return -1;
    }

    if(mode == F_OK) {
        return 0;
    }

    if((mode & R_OK) && !(st.st_mode & S_IRUSR)) {
        errno = EACCES;
        return -1;
    }

    if((mode & W_OK) && !(st.st_mode & S_IWUSR)) {
        errno = EACCES;
        return -1;
    }

    /* Directories need to be searchable for path traversal to work; KOS
     * marks them S_IRWXU, so this falls out of the ordinary bit test. */
    if((mode & X_OK) && !(st.st_mode & S_IXUSR)) {
        errno = EACCES;
        return -1;
    }

    return 0;
}

/* No user or group model: everything runs as a single privileged user, so
 * report the same id consistently rather than failing. glib uses these to
 * decide things like whether a directory is "ours", which is trivially
 * true here. */
uid_t getuid(void) {
    return 0;
}

uid_t geteuid(void) {
    return 0;
}

gid_t getgid(void) {
    return 0;
}

gid_t getegid(void) {
    return 0;
}

/* No process hierarchy either. 1 is the conventional "init" answer and
 * keeps callers that sanity-check for a plausible pid happy. */
pid_t getppid(void) {
    return 1;
}

/* KOS has no signal delivery, so there is never anything to block or
 * unblock. glib's main loop calls this around its child-watch handling --
 * which also can't happen here, there being no child processes -- so
 * reporting success and doing nothing is accurate: the resulting mask
 * really is empty. Filling in *oldset when asked keeps callers that
 * save-and-restore it well behaved. */
int pthread_sigmask(int how, const sigset_t* set, sigset_t* oldset) {
    (void) how;
    (void) set;

    if(oldset) {
        __builtin_memset(oldset, 0, sizeof(*oldset));
    }

    return 0;
}
