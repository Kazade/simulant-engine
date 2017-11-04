#include "global.h"

#ifdef _arch_dreamcast
smlt::KOSWindow::ptr window;
#else
smlt::SDL2Window::ptr window;
#endif
