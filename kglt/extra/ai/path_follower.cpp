#include "../../scene.h"
#include "../../window.h"
#include "../../actor.h"

#include "path_follower.h"

namespace kglt {
namespace extra {

PathFollower::PathFollower(ActorHolder *parent, float max_speed, float max_force):
    actor_(parent),
    max_speed_(max_speed),
    max_force_(max_force) {

    parent->scene().window().signal_step().connect(std::bind(&PathFollower::_update, this, std::placeholders::_1));
}

float map(float value, float min, float max, float new_min, float new_max) {
    float percent = value / (max - min);
    return new_min + ((new_max - new_min) * percent);
}

kglt::Vec3 PathFollower::force_to_apply(const Vec3 &velocity) const {
    return force_to_apply_;
}

kglt::Vec3 PathFollower::forward() {
    kglt::Vec3 result;
    kmQuaternionGetForwardVec3RH(&result, &actor_->actor()->absolute_rotation());
    return result;
}

void PathFollower::seek(const kglt::Vec3& target) {
    kglt::Vec3 desired = target - actor_->actor()->absolute_position();

    float d = desired.length();
    desired.normalize();

    float slow_down_radius = 1.0;
    if(d < slow_down_radius) {
        //Slow down as we approach the target
        float m = map(d, 0, slow_down_radius, 0, max_speed_);
        desired = desired * m;
    } else {
        desired = desired * max_speed_;
    }

    kglt::Vec3 steer = desired - forward();

    steer.limit(max_force_);

    force_to_apply_ = steer;
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

kglt::Vec3 PathFollower::get_normal_point(const kglt::Vec3& p, const kglt::Vec3& a, const kglt::Vec3& b) {
    kglt::Vec3 ap = p - a;
    kglt::Vec3 ab = b - a;

    ab.normalize();
    ab = ab * (ap.dot(ab));

    kglt::Vec3 normal_point = a + ab;

    return normal_point;
}

bool PathFollower::point_on_line(const kglt::Vec3& p, const kglt::Vec3& a, const kglt::Vec3& b) {
    float v1 = (p.x - a.x) / (b.x - a.x);
    float v2 = (p.y - a.y) / (b.y - a.y);
    float v3 = (p.z - a.z) / (b.z - a.z);

    return kmAlmostEqual(v1, v2) && kmAlmostEqual(v2, v3);
}

void PathFollower::_update(double dt) {
    if(path_.empty()) {
        return;
    }

    kglt::Vec3 predict = forward();
    predict.normalize();
    predict = predict * 5.0;

    kglt::Vec3 predict_loc = kglt::Vec3(actor_->actor()->absolute_position()) + predict;

    kglt::Vec3 target;
    float closest = 100000;
    for(int i = 0; i < path_.length() - 1; ++i) {
        kglt::Vec3 a = path_.point(i);
        kglt::Vec3 b = path_.point(i+1);
        kglt::Vec3 normal_point = get_normal_point(predict_loc, a, b);

        //If the point isn't on the line, then just use
        //the line segment end point
        if(!point_on_line(normal_point, a, b)) {
            normal_point = b;
        }

        float dist = (predict_loc - normal_point).length();
        if(dist < closest) {
            target = normal_point;
            closest = dist;
        }
    }

    float distance = (target - predict_loc).length();

    if(distance > path_.radius()) {
        seek(target);
    }
}

}
}
