/* PSPSDK's newlib *declares* these in the usual headers (sys/unistd.h,
 * signal.h, sys/wait.h, pwd.h) -- confirmed cross-compiling glib for
 * PSP: it's only the *implementations* that are missing, so glib itself
 * compiles fine and this only shows up as undefined references at the
 * final executable link (linking is per-object-file; something in
 * libglib-2.0.a calling any of these pulls in the whole containing .o,
 * even though Simulant's generated Vala code never exercises the
 * process/signal/user-database machinery these back). There's no real
 * syscall behind any of them on a single-process console OS with no
 * user/group model -- each stub below fails the way glib's own calling
 * code already handles a genuinely unsupported platform (glib never
 * treats these as hard requirements; every caller has a fallback for
 * "this returned an error").
 *
 * _flush_cache is different: it's a real, correctly-implemented function
 * (not a stub) via the PSP's actual cache-management syscalls, needed by
 * libffi's MIPS closure trampoline (see cmake/psp-shims/sgidefs.h and
 * cmake/glib-patches/psp/libffi/fix-allegrex-fpu-ops.patch for the two
 * other libffi/PSP caveats already documented) to make newly-written
 * executable code visible to the instruction cache -- standard practice
 * on any Harvard-cache MIPS target, this one included.
 */
#include <errno.h>
#include <pspkernel.h>
#include <psputils.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int _flush_cache(char* addr, int nbytes, int cache) {
    (void) cache;
    sceKernelDcacheWritebackInvalidateRange(addr, (unsigned int) nbytes);
    sceKernelIcacheInvalidateRange(addr, (unsigned int) nbytes);
    return 0;
}

pid_t getppid(void) {
    return 1;
}

int dup2(int oldfd, int newfd) {
    (void) oldfd;
    (void) newfd;
    errno = ENOSYS;
    return -1;
}

long sysconf(int name) {
    (void) name;
    errno = EINVAL;
    return -1;
}

gid_t getgid(void) {
    return 0;
}

gid_t getegid(void) {
    return 0;
}

int ftruncate(int fd, off_t length) {
    (void) fd;
    (void) length;
    errno = ENOSYS;
    return -1;
}

int sigaction(int sig, const struct sigaction* act, struct sigaction* oact) {
    (void) sig;
    (void) act;
    (void) oact;
    errno = ENOSYS;
    return -1;
}

pid_t waitpid(pid_t pid, int* status, int options) {
    (void) pid;
    (void) status;
    (void) options;
    errno = ECHILD;  /* no such thing as a child process on PSP */
    return -1;
}

int getpwnam_r(const char* name, struct passwd* pwd, char* buf, size_t buflen,
               struct passwd** result) {
    (void) name;
    (void) pwd;
    (void) buf;
    (void) buflen;
    *result = NULL;  /* no user database on PSP -- "not found", not an error */
    return 0;
}
