/* A small iconv() for Dreamcast.
 *
 * KOS's newlib declares iconv_open/iconv/iconv_close in <iconv.h> but is
 * built without --enable-newlib-iconv, so libc.a contains none of them.
 * glib has no option to go without: outside Windows (where it compiles in
 * its own win_iconv.c) its meson.build does a bare dependency('iconv') and
 * stops the configure if that fails.
 *
 * Rather than pull in a full libiconv for a target whose text is
 * essentially all UTF-8, this covers the encodings that actually come up --
 * UTF-8, Latin-1 and ASCII -- and cleanly refuses anything else, so glib
 * reports a normal "conversion not supported" GError instead of silently
 * producing wrong text. Everything routes through a Unicode code point, so
 * any supported pair works in both directions.
 *
 * glib's UTF-8 <-> UTF-16 <-> UCS-4 conversions don't come through here at
 * all; those are native to gutf8.c.
 */
#include <errno.h>
#include <iconv.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    CS_UTF8,
    CS_LATIN1,
    CS_ASCII
} charset_t;

typedef struct {
    charset_t from;
    charset_t to;
} dc_iconv_t;

/* Returns 0 and sets *out on success, non-zero if we don't handle it. */
static int charset_lookup(const char* name, charset_t* out) {
    /* Charset names are case-insensitive, and callers spell them many ways
     * ("UTF-8", "utf8", ...). Also tolerate glib's "//TRANSLIT" and
     * "//IGNORE" suffixes by matching only up to the marker -- we don't
     * implement either behaviour, but the base encoding is still right. */
    size_t len;
    const char* slashes;
    char buf[32];
    size_t i;

    if(!name) {
        return -1;
    }

    slashes = strstr(name, "//");
    len = slashes ? (size_t) (slashes - name) : strlen(name);
    if(len >= sizeof(buf)) {
        return -1;
    }

    for(i = 0; i < len; i++) {
        char c = name[i];
        if(c >= 'a' && c <= 'z') {
            c = (char) (c - 'a' + 'A');
        }
        buf[i] = c;
    }
    buf[len] = '\0';

    if(!strcmp(buf, "UTF-8") || !strcmp(buf, "UTF8")) {
        *out = CS_UTF8;
        return 0;
    }

    if(!strcmp(buf, "ISO-8859-1") || !strcmp(buf, "ISO8859-1") ||
       !strcmp(buf, "LATIN1") || !strcmp(buf, "ISO_8859-1")) {
        *out = CS_LATIN1;
        return 0;
    }

    if(!strcmp(buf, "ASCII") || !strcmp(buf, "US-ASCII") ||
       !strcmp(buf, "ANSI_X3.4-1968") || !strcmp(buf, "ANSI_X3.4-1986")) {
        *out = CS_ASCII;
        return 0;
    }

    return -1;
}

iconv_t iconv_open(const char* to, const char* from) {
    charset_t to_cs, from_cs;
    dc_iconv_t* cd;

    if(charset_lookup(to, &to_cs) != 0 || charset_lookup(from, &from_cs) != 0) {
        errno = EINVAL;
        return (iconv_t) -1;
    }

    cd = (dc_iconv_t*) malloc(sizeof(dc_iconv_t));
    if(!cd) {
        errno = ENOMEM;
        return (iconv_t) -1;
    }

    cd->from = from_cs;
    cd->to = to_cs;
    return (iconv_t) cd;
}

int iconv_close(iconv_t cd) {
    if(cd != (iconv_t) -1) {
        free(cd);
    }
    return 0;
}

/* Pulls one code point off the input.
 * Returns bytes consumed, 0 if the input is a valid but incomplete
 * sequence (caller reports EINVAL), or -1 if it's malformed (EILSEQ). */
static int decode(charset_t cs, const unsigned char* in, size_t avail,
                  unsigned long* cp) {
    unsigned char c = in[0];
    int extra, i;

    if(cs == CS_LATIN1) {
        *cp = c;
        return 1;
    }

    if(cs == CS_ASCII) {
        if(c >= 0x80) {
            return -1;
        }
        *cp = c;
        return 1;
    }

    /* UTF-8 */
    if(c < 0x80) {
        *cp = c;
        return 1;
    } else if((c & 0xE0) == 0xC0) {
        extra = 1;
        *cp = c & 0x1F;
    } else if((c & 0xF0) == 0xE0) {
        extra = 2;
        *cp = c & 0x0F;
    } else if((c & 0xF8) == 0xF0) {
        extra = 3;
        *cp = c & 0x07;
    } else {
        return -1; /* continuation byte or 5/6-byte form */
    }

    if(avail < (size_t) (extra + 1)) {
        return 0; /* incomplete, but not yet wrong */
    }

    for(i = 1; i <= extra; i++) {
        if((in[i] & 0xC0) != 0x80) {
            return -1;
        }
        *cp = (*cp << 6) | (unsigned long) (in[i] & 0x3F);
    }

    /* Reject over-long forms and anything outside Unicode, so bad input is
     * an error here rather than something odd downstream. */
    if((extra == 1 && *cp < 0x80) || (extra == 2 && *cp < 0x800) ||
       (extra == 3 && *cp < 0x10000) || *cp > 0x10FFFF ||
       (*cp >= 0xD800 && *cp <= 0xDFFF)) {
        return -1;
    }

    return extra + 1;
}

/* Writes one code point.
 * Returns bytes written, 0 if there isn't room (caller reports E2BIG), or
 * -1 if the code point has no representation in this charset (EILSEQ). */
static int encode(charset_t cs, unsigned long cp, unsigned char* out,
                  size_t space) {
    if(cs == CS_LATIN1 || cs == CS_ASCII) {
        unsigned long limit = (cs == CS_ASCII) ? 0x80 : 0x100;
        if(cp >= limit) {
            return -1;
        }
        if(space < 1) {
            return 0;
        }
        out[0] = (unsigned char) cp;
        return 1;
    }

    /* UTF-8 */
    if(cp < 0x80) {
        if(space < 1) {
            return 0;
        }
        out[0] = (unsigned char) cp;
        return 1;
    } else if(cp < 0x800) {
        if(space < 2) {
            return 0;
        }
        out[0] = (unsigned char) (0xC0 | (cp >> 6));
        out[1] = (unsigned char) (0x80 | (cp & 0x3F));
        return 2;
    } else if(cp < 0x10000) {
        if(space < 3) {
            return 0;
        }
        out[0] = (unsigned char) (0xE0 | (cp >> 12));
        out[1] = (unsigned char) (0x80 | ((cp >> 6) & 0x3F));
        out[2] = (unsigned char) (0x80 | (cp & 0x3F));
        return 3;
    }

    if(space < 4) {
        return 0;
    }
    out[0] = (unsigned char) (0xF0 | (cp >> 18));
    out[1] = (unsigned char) (0x80 | ((cp >> 12) & 0x3F));
    out[2] = (unsigned char) (0x80 | ((cp >> 6) & 0x3F));
    out[3] = (unsigned char) (0x80 | (cp & 0x3F));
    return 4;
}

size_t iconv(iconv_t cd, char** inbuf, size_t* inbytesleft, char** outbuf,
             size_t* outbytesleft) {
    dc_iconv_t* state = (dc_iconv_t*) cd;

    if(!state || cd == (iconv_t) -1) {
        errno = EBADF;
        return (size_t) -1;
    }

    /* A NULL/empty inbuf means "reset to the initial state". These are all
     * stateless encodings, so there's nothing to reset. */
    if(!inbuf || !*inbuf || !inbytesleft || *inbytesleft == 0) {
        return 0;
    }

    while(*inbytesleft > 0) {
        unsigned long cp;
        int consumed = decode(state->from, (const unsigned char*) *inbuf,
                              *inbytesleft, &cp);
        int written;

        if(consumed < 0) {
            errno = EILSEQ;
            return (size_t) -1;
        }
        if(consumed == 0) {
            errno = EINVAL; /* truncated sequence at end of input */
            return (size_t) -1;
        }

        if(!outbuf || !*outbuf || !outbytesleft) {
            errno = E2BIG;
            return (size_t) -1;
        }

        written = encode(state->to, cp, (unsigned char*) *outbuf,
                         *outbytesleft);
        if(written < 0) {
            errno = EILSEQ; /* not representable in the target charset */
            return (size_t) -1;
        }
        if(written == 0) {
            errno = E2BIG;
            return (size_t) -1;
        }

        *inbuf += consumed;
        *inbytesleft -= (size_t) consumed;
        *outbuf += written;
        *outbytesleft -= (size_t) written;
    }

    /* No characters were converted in a non-reversible way. */
    return 0;
}
