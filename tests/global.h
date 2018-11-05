#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef _arch_dreamcast
#include "simulant/kos_window.h"
extern smlt::KOSWindow::ptr window;
#else
#include "simulant/sdl2_window.h"
extern smlt::SDL2Window::ptr window;
#endif

#include "kaztest/kaztest.h"
#include "simulant/window.h"
#include "simulant/stage.h"

class SimulantTestCase : public TestCase {
public:
    void set_up();
};


#endif // GLOBAL_H
