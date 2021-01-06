#ifndef KEYCODES_H
#define KEYCODES_H

#ifdef __DREAMCAST__
    #include "kos_keycodes.h"
#elif defined(__PSP__)
    #include "platforms/psp/psp_keycodes.h"
#else
    #include "sdl2_keycodes.h"
#endif

#endif // KEYCODES_H
