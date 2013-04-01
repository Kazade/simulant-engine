#include "renderer.h"
#include "scene.h"

namespace kglt {

SubScene& Renderer::current_subscene() { return scene().subscene(current_subscene_); }


}
