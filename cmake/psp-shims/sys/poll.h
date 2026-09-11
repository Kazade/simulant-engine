/* PSPSDK's newlib has no <sys/poll.h> at all (confirmed by cross-compiling
 * deps/glib against it: glib's meson.build hard-errors at configure time
 * with "FIX POLL* defines" if neither <sys/poll.h> nor <winsock2.h> is
 * found -- see the has_syspoll/has_winsock2 check there). glib/gmain.c and
 * glib/gpoll.c (both compiled unconditionally, not part of GIO/GModule)
 * need the POLL* constants and a poll() to exist to compile at all, even
 * though nothing Simulant's generated Vala code does ever drives a real
 * GMainLoop over PSP network sockets at runtime -- this only needs to be
 * *correct enough to compile and not misbehave*, not exercised in anger.
 *
 * PSPSDK does provide a real <sys/select.h> (fd_set/select()/FD_* macros),
 * so poll() is implemented directly in terms of it below -- the standard
 * compatibility shim used on any POSIX-ish libc that has select() but not
 * poll() (this is not a PSP-specific trick).
 *
 * The POLL* bit values themselves don't need to match any real kernel
 * ABI (there's no actual poll(2) syscall on the PSP for them to agree
 * with) -- chosen to match the historical Linux/BSD values, both because
 * that's the value every other libc's <poll.h> uses and because it
 * happens to already match PSPSDK's own (differently-named)
 * SCE_NET_INET_POLLIN et al in pspnet_inet.h.
 *
 * See cmake/GLibCrossFilePSP.cmake, which adds this directory to the
 * PSP cross-compiler's include path when building deps/glib.
 */
#ifndef SIMULANT_PSP_SYS_POLL_H
#define SIMULANT_PSP_SYS_POLL_H

#include <sys/select.h>
#include <sys/time.h>

#define POLLIN     0x0001
#define POLLPRI    0x0002
#define POLLOUT    0x0004
#define POLLERR    0x0008
#define POLLHUP    0x0010
#define POLLNVAL   0x0020

typedef unsigned long nfds_t;

struct pollfd {
    int fd;
    short events;
    short revents;
};

static __inline__ int poll(struct pollfd* fds, nfds_t nfds, int timeout_ms) {
    fd_set readfds, writefds, exceptfds;
    struct timeval tv;
    struct timeval* tvp = NULL;
    int maxfd = -1;
    nfds_t i;
    int ready;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    for(i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if(fds[i].fd < 0) {
            continue;
        }
        if(fds[i].events & (POLLIN | POLLPRI)) {
            FD_SET(fds[i].fd, &readfds);
        }
        if(fds[i].events & POLLOUT) {
            FD_SET(fds[i].fd, &writefds);
        }
        FD_SET(fds[i].fd, &exceptfds);
        if(fds[i].fd > maxfd) {
            maxfd = fds[i].fd;
        }
    }

    if(timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tvp = &tv;
    }

    ready = select(maxfd + 1, &readfds, &writefds, &exceptfds, tvp);
    if(ready <= 0) {
        return ready;
    }

    ready = 0;
    for(i = 0; i < nfds; i++) {
        if(fds[i].fd < 0) {
            continue;
        }
        if(FD_ISSET(fds[i].fd, &readfds)) {
            fds[i].revents |= POLLIN;
        }
        if(FD_ISSET(fds[i].fd, &writefds)) {
            fds[i].revents |= POLLOUT;
        }
        if(FD_ISSET(fds[i].fd, &exceptfds)) {
            fds[i].revents |= POLLERR;
        }
        if(fds[i].revents != 0) {
            ready++;
        }
    }
    return ready;
}

#endif
