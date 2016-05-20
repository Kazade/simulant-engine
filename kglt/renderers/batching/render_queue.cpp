
#include "../../stage.h"
#include "../../material.h"
#include "../../actor.h"

#include "render_queue.h"
#include "../../partitioner.h"
#include "../../partitioners/static_chunk.h"

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

    stage->signal_actor_destroyed().connect([=](ActorID actor_id) {
        auto actor = stage->actor(actor_id);
        actor->each([=](uint32_t i, SubActor* subactor) {
            remove_renderable(subactor);
        });
    });

    stage->signal_actor_changed().connect([=](ActorID actor_id, ActorChangeEvent event) {
        auto actor = stage->actor(actor_id);
        if(event.type == ACTOR_CHANGE_TYPE_SUBACTOR_MATERIAL_CHANGED) {
            actor->each([=](uint32_t i, SubActor* subactor) {
                remove_renderable(subactor);
                insert_renderable(subactor);
            });
        }
    });

    stage->partitioner->signal_static_chunk_created().connect([=](StaticChunk* chunk) {
        chunk->each([=](uint32_t i, StaticSubchunk* subchunk) {
            insert_renderable(subchunk);
        });
    });

    stage->partitioner->signal_static_chunk_destroyed().connect([=](StaticChunk* chunk) {
        chunk->each([=](uint32_t i, StaticSubchunk* subchunk) {
            remove_renderable(subchunk);
        });
    });

    stage->partitioner->signal_static_chunk_changed().connect([=](StaticChunk* chunk, StaticChunkChangeEvent event) {
        if(event.type == STATIC_CHUNK_CHANGE_TYPE_SUBCHUNK_CREATED) {
            insert_renderable(event.subchunk_created.subchunk);
        } else {
            assert(event.type == STATIC_CHUNK_CHANGE_TYPE_SUBCHUNK_DESTROYED);
            remove_renderable(event.subchunk_destroyed.subchunk);
        }
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

void RenderQueue::clean_empty_batches() {
    for(auto pass = batches_.begin(); pass != batches_.end();) {
        for(auto group = pass->begin(); group != pass->end();) {
            if(!group->second.renderable_count()) {
                pass->erase(group++);
            } else {
                ++group;
            }
        }

        if(pass->empty()) {
            pass = batches_.erase(pass);
        } else {
            ++pass;
        }
    }
}

void RenderQueue::remove_renderable(Renderable* renderable) {
    /*
     * The Renderable is a BatchMember had maintains a list of
     * batches that it is a member of. This way removing the renderable
     * is just a case of iterating its batches and removing it. If the
     * renderable was the last in the batch we need to remove the batch.
     */

    bool empty_batches_created = false;
    for(auto batch: renderable->batches()) {
        batch->remove_renderable(renderable);
        if(!batch->renderable_count()) {
            empty_batches_created = true;
        }
    }

    if(empty_batches_created) {
        clean_empty_batches();
    }
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

void Batch::add_renderable(Renderable* renderable) {
    renderable->join_batch(this);
    renderables_.push_back(renderable);
}

void Batch::remove_renderable(Renderable *renderable) {
    auto it = std::find(renderables_.begin(), renderables_.end(), renderable);
    if(it != renderables_.end()) {
        renderables_.erase(it);
    }
    renderable->leave_batch(this);
}

}
}
