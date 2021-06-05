

#include "platform.h"

#ifdef __DREAMCAST__

#include "platforms/dreamcast/platform.h"
typedef smlt::DreamcastPlatform ThisPlatform;

#elif defined(__PSP__)

#include "platforms/psp/platform.h"
typedef smlt::PSPPlatform ThisPlatform;

#elif defined(__WIN32__) || defined(__WIN64__)

#include "platforms/windows/platform.h"
typedef smlt::WindowsPlatform ThisPlatform;

#elif defined(__ANDROID__)

#include "platforms/android/platform.h"
typedef smlt::AndroidPlatform ThisPlatform;

#elif defined(__linux__)

#include "platforms/linux/platform.h"
typedef smlt::LinuxPlatform ThisPlatform;

#elif defined(__APPLE__)

#include "platforms/osx/platform.h"
typedef smlt::OSXPlatform ThisPlatform;

#else
#error Unrecognised platform
#endif


namespace smlt {

Platform* get_platform()  {
    static ThisPlatform platform;
    return &platform;
}

}
