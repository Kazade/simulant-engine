#ifndef DEBUG_BAR_H
#define DEBUG_BAR_H

#include <SDL/SDL.h>
#include <AntTweakBar.h>
#include <kazmath/kazmath.h>

#include "../generic/managed.h"
#include "../kazbase/unicode.h"

namespace kglt {

class DebugBar:
    public Managed<DebugBar> {

public:
    DebugBar(int32_t window_width, int32_t window_height);
    ~DebugBar();

    void add_read_only_variable(const unicode& id, bool* variable);
    void add_read_only_variable(const unicode& id, int32_t* variable);
    void add_read_only_variable(const unicode& id, float* variable);
    void add_read_only_variable(const unicode& id, double* variable);
    void add_read_only_variable(const unicode &id, kmVec3 *variable);
    void add_read_only_variable(const unicode& id, kmQuaternion* variable);

    bool handle_event(SDL_Event& event);
    void render();
    void toggle() { visible_ = !visible_; }
private:
    TwBar* ant_bar_;
    bool visible_;
};

}

#endif // DEBUG_BAR_H
