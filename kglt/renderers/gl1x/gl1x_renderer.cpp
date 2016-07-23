
#include "gl1x_renderer.h"

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

}
