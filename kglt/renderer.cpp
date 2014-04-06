#include "renderer.h"
#include "scene.h"

namespace kglt {

AutoWeakPtr<Stage> Renderer::current_stage() { return scene().stage(current_stage_); }


}
