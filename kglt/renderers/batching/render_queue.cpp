#include "../../stage.h"
#include "../../material.h"
#include "../../actor.h"

#include "render_queue.h"

namespace kglt {
namespace new_batcher {

RenderQueue::RenderQueue(Stage* stage, RenderGroupFactory* render_group_factory):
    stage_(stage),
    render_group_factory_(render_group_factory) {

    stage->signal_actor_created().connect([=](ActorID actor_id) {
        auto actor = stage->actor(actor_id);
        actor->each([=](uint32_t i, SubActor* subactor) {
            insert_renderable(subactor);
        });
    });
}

void RenderQueue::insert_renderable(Renderable* renderable) {
    /*
     * Adds a renderable to the correct render groups. This goes through the
     * material passes on the renderable, calculates the render group for each one
     * and then adds the renderable to that pass's render queue
     */

    auto material_id = renderable->material_id();
    auto material = stage_->material(material_id);

    material->each([&](uint32_t i, MaterialPass* material_pass) {
        RenderGroup group = render_group_factory_->new_render_group(
            renderable, material_pass
        );

        assert(i < MAX_MATERIAL_PASSES);
        assert(i < material->pass_count());

        if(batches_.size() <= i) {
            batches_.push_back(BatchMap());
        }

        batches_[i][group].add_renderable(renderable);
    });
}

void RenderQueue::remove_renderable(Renderable* renderable) {
    assert(0 && "Not Implemented");
}

void RenderQueue::traverse(TraverseCallback callback, uint64_t frame_id) const {
    const RenderGroup* last_group = nullptr;

    Pass pass = 0;
    for(auto& batches: batches_) {
        MaterialPtr material;
        MaterialPass::ptr material_pass;
        IterationType pass_iteration_type;

        for(auto& p: batches) {
            const RenderGroup* current_group = &p.first;

            p.second.each([&](uint32_t i, Renderable* renderable) {
                if(!renderable->is_visible_in_frame(frame_id)) {
                    return;
                }

                // Only look up the material and pass if we have to (if it changed)
                if(!material || material->id() != renderable->material_id()) {
                    material = stage_->material(renderable->material_id());
                    assert(material);

                    material_pass = material->pass(pass);
                    pass_iteration_type = material_pass->iteration();
                }

                switch(pass_iteration_type) {
                    case ITERATE_N: {
                        for(Iteration i = 0; i < material_pass->max_iterations(); ++i) {
                            callback(current_group, last_group, renderable, material_pass.get(), i);
                        }
                    }
                    break;
                    case ITERATE_ONCE_PER_LIGHT:
                        assert(0 && "Not Implemented");
                    break;
                    default:
                        callback(current_group, last_group, renderable, material_pass.get(), 0);
                }
            });

            last_group = current_group;
        }
        ++pass;
    }
}

}
}
