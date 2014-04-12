#include "renderer.h"
#include "scene.h"

namespace kglt {

StagePtr Renderer::current_stage() { return window().stage(current_stage_); }


}
