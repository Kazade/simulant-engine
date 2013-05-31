#include "renderer.h"
#include "scene.h"

namespace kglt {

Stage& Renderer::current_subscene() { return scene().subscene(current_subscene_); }


}
