#include "renderer.h"

namespace kglt {

StagePtr Renderer::current_stage() { return window->stage(current_stage_); }


}
