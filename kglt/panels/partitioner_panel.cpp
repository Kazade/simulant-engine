
#include "partitioner_panel.h"
#include "../window_base.h"
#include "../stage.h"
#include "../actor.h"
#include "../partitioner.h"

namespace kglt {

PartitionerPanel::PartitionerPanel(WindowBase* window):
    window_(window) {

    window_->signal_stage_removed().connect([=](StageID stage_id) {
        debug_actors_.erase(stage_id);
    });
}

void PartitionerPanel::do_activate() {
    initialize();

    for(auto& pair: debug_actors_) {
        window_->stage(pair.first)->actor(pair.second)->set_visible(true);
    }
}

void PartitionerPanel::do_deactivate() {
    for(auto& pair: debug_actors_) {
        window_->stage(pair.first)->actor(pair.second)->set_visible(false);
    }
}

void PartitionerPanel::initialize() {
    if(initialized_) return;

    /* for stage in stages: stage->new_actor(stage->partitioner->debug_mesh_id()) */
    window_->StageManager::each([=](Stage* stage) {
        debug_actors_[stage->id()] = stage->new_actor_with_mesh(stage->partitioner->debug_mesh_id());
    });

    initialized_ = true;
}


}
