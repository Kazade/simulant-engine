#include "renderer.h"
#include "scene.h"

namespace kglt {

void Renderer::render(const std::vector<GeometryBuffer::ptr>& buffers) { //FIXME: Should pass in batching structure
    for(GeometryBuffer::ptr buffer: buffers) {
        render_buffer(*buffer);
    }
}

}
