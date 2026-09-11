/* PSPSDK's newlib has no <sys/utsname.h>/uname() at all (there's no "OS
 * name/version" concept on the PSP to report). glib/gutils.c's
 * get_os_info_from_uname() already handles uname() failing gracefully --
 * `if (uname(&info) == -1) return NULL;` -- so a stub that always fails
 * is exactly the behavior glib already expects from a platform that
 * can't answer this, not a special case.
 */
#ifndef SIMULANT_PSP_SYS_UTSNAME_H
#define SIMULANT_PSP_SYS_UTSNAME_H

struct utsname {
    char sysname[1];
    char nodename[1];
    char release[1];
    char version[1];
    char machine[1];
};

static __inline__ int uname(struct utsname* buf) {
    (void) buf;
    return -1;
}

#endif
