/* KOS provides a complete poll() -- struct pollfd, nfds_t, the POLL*
 * constants and the function itself -- but at <poll.h>, with no <sys/poll.h>
 * alias. glib checks for and includes the sys/ spelling (see the
 * has_syspoll branch in glib's meson.build, which otherwise stops the build
 * with "FIX POLL* defines"), so this just forwards.
 *
 * Deliberately a forward rather than a reimplementation: unlike PSP, where
 * <sys/poll.h> is absent entirely and cmake/psp-shims/sys/poll.h has to
 * build poll() out of select(), here the real thing already exists. */
#ifndef __SIMULANT_DREAMCAST_SYS_POLL_H__
#define __SIMULANT_DREAMCAST_SYS_POLL_H__

#include <poll.h>

#endif
