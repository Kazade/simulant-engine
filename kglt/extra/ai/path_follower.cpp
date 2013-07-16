#include "../../scene.h"
#include "../../window.h"
#include "../../actor.h"

#include "path_follower.h"

namespace kglt {
namespace extra {

PathFollower::PathFollower(ActorHolder *parent):
    actor_(parent) {
    parent->scene().window().signal_step().connect(std::bind(&PathFollower::_update, this, std::placeholders::_1));
}

void PathFollower::follow(Path path) {
    assert(actor_);

    path_ = path;

    //Move the actor directly to the first waypoint
    actor_->actor()->move_to(path_.point(0));
}

void PathFollower::enable_debug(bool value) {
    assert(actor_);
    Stage* stage = actor_->stage();
    assert(stage);

    if(value) {
        if(!debug_mesh_) {
            debug_mesh_ = stage->new_mesh();
        }

        auto mesh = stage->mesh(debug_mesh_).lock();
        mesh->clear();

        auto smi = mesh->new_submesh(kglt::MaterialID(), MESH_ARRANGEMENT_LINE_STRIP);

        for(uint32_t i = 0; i < path_.length(); ++i) {
            mesh->shared_data().position(path_.point(i));
            mesh->shared_data().diffuse(kglt::Colour::blue);
            mesh->shared_data().tex_coord0(kglt::Vec2());
            mesh->shared_data().tex_coord1(kglt::Vec2());
            mesh->shared_data().normal(kglt::Vec3());
            mesh->shared_data().move_next();

            mesh->submesh(smi).index_data().index(i);
        }

        mesh->shared_data().done();
        mesh->submesh(smi).index_data().done();

        if(!debug_actor_) {
            debug_actor_ = stage->new_actor(debug_mesh_);
        }
    } else {
        stage->delete_actor(debug_actor_);

        debug_actor_ = kglt::ActorID();
        debug_mesh_ = kglt::MeshID();
    }
}

void PathFollower::_update(double dt) {
    if(path_.empty()) {
        return;
    }


}

}
}
