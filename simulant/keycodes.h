#pragma once

#ifdef __DREAMCAST__
    #include "kos_keycodes.h"
#elif defined(__PSP__)
    #include "platforms/psp/psp_keycodes.h"
#elif defined(__ANDROID__)
    #include "platforms/android/android_keycodes.h"
#else
    #include "sdl2_keycodes.h"
#endif
