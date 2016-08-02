#ifdef KGLT_GL_VERSION_1X


#include "gl1x_renderer.h"
#include "./glad/glad/glad.h"
#include "../../utils/gl_error.h"

namespace kglt {

class GL1RenderGroupImpl: public batcher::RenderGroupImpl {
public:
    GL1RenderGroupImpl(RenderPriority priority):
        batcher::RenderGroupImpl(priority) {}

    TextureID texture_id[MAX_TEXTURE_UNITS] = {TextureID()};

    bool lt(const RenderGroupImpl& other) const override {
        const GL1RenderGroupImpl* rhs = dynamic_cast<const GL1RenderGroupImpl*>(&other);
        if(!rhs) {
            // Should never happen... throw an error maybe?
            return false;
        }

        for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            if(texture_id[i].value() < rhs->texture_id[i].value()) {
                return true;
            }
        }

        return false;
    }
};

batcher::RenderGroup GL1XRenderer::new_render_group(Renderable *renderable, MaterialPass *material_pass) {
    auto impl = std::make_shared<GL1RenderGroupImpl>(renderable->render_priority());

    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        if(i < material_pass->texture_unit_count()) {
            auto tex_id = material_pass->texture_unit(i).texture_id();
            impl->texture_id[i] = tex_id;
        } else {
            impl->texture_id[i] = TextureID();
        }
    }

    return batcher::RenderGroup(impl);
}

void set_transformation_matrices(CameraPtr camera, Renderable* renderable) {

}

void set_material_properties(MaterialPass* material_pass) {

}

void set_light_properties(Light* light, batcher::Iteration iteration) {

}

void send_geometry(Renderable* renderable) {
    std::size_t index_count = renderable->index_data->count();
    if(!index_count) {
        return;
    }

    /*
    switch(renderable->arrangement()) {
        case MESH_ARRANGEMENT_POINTS:
            GLCheck(glDrawElements, GL_POINTS, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINES:
            GLCheck(glDrawElements, GL_LINES, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINE_STRIP:
            GLCheck(glDrawElements, GL_LINE_STRIP, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLES:
            GLCheck(glDrawElements, GL_TRIANGLES, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_STRIP:
            GLCheck(glDrawElements, GL_TRIANGLE_STRIP, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_FAN:
            GLCheck(glDrawElements, GL_TRIANGLE_FAN, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        default:
            throw NotImplementedError(__FILE__, __LINE__);
    }*/
}

void GL1XRenderer::render(CameraPtr camera, StagePtr stage, bool render_group_changed,
    const batcher::RenderGroup*, Renderable* renderable, MaterialPass* material_pass, Light* light, batcher::Iteration iteration) {

    set_transformation_matrices(camera, renderable);
    set_material_properties(material_pass);

    if(light) {
        set_light_properties(light, iteration);
    }

    send_geometry(renderable);
}

void GL1XRenderer::init_context() {
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 1.1");
    }

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);
}


}

#endif // KGLT_GL_VERSION_1X
