#include "renderer.h"
#include "scene.h"

namespace kglt {

StagePtr Renderer::current_stage() { return scene().stage(current_stage_); }


}
