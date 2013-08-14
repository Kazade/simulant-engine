#include "../../scene.h"
#include "../../window.h"
#include "../../actor.h"

#include "boid.h"

namespace kglt {
namespace extra {

Boid::Boid(MoveableActorHolder *parent, float max_speed, float max_force):
    actor_(parent) {

}

float map(float value, float min, float max, float new_min, float new_max) {
    float percent = value / (max - min);
    return new_min + ((new_max - new_min) * percent);
}


kglt::Vec3 get_normal_point(const kglt::Vec3& p, const kglt::Vec3& a, const kglt::Vec3& b) {
    kglt::Vec3 ap = p - a;
    kglt::Vec3 ab = b - a;

    ab.normalize();
    ab = ab * (ap.dot(ab));

    kglt::Vec3 normal_point = a + ab;

    return normal_point;
}

bool point_on_line(const kglt::Vec3& p, const kglt::Vec3& a, const kglt::Vec3& b) {
    double ab = (b - a).length();
    double ap = (p - a).length();
    double pb = (b - p).length();

    return kmAlmostEqual(ab, (ap + pb));
}

Vec3 Boid::pursue(const kglt::Vec3& target, const kglt::Vec3& target_velocity, const float target_max_speed) const {
    float T = kglt::Vec3::distance(target, actor_->position()) / target_max_speed;
    kglt::Vec3 target_location = target + (target_velocity * T);
    return seek(target_location);
}

kglt::Vec3 Boid::evade(const Vec3& target, const Vec3& target_velocity, const float target_max_speed) const {
    float T = kglt::Vec3::distance(target, actor_->position()) / target_max_speed;
    kglt::Vec3 target_location = target + (target_velocity * T);
    return flee(target_location);
}

Vec3 Boid::seek(const kglt::Vec3& target, float slowing_radius) const {
    kglt::Vec3 desired_velocity = (target - actor_->position());

    float distance = desired_velocity.length();
    desired_velocity.normalize();

    if(distance < slowing_radius) {
        float m = map(distance, 0, slowing_radius, actor_->min_speed(), actor_->max_speed());
        desired_velocity = desired_velocity * m;
    } else {
        desired_velocity = desired_velocity * actor_->max_speed();
    }

    kglt::Vec3 steering = desired_velocity - actor_->velocity();

    steering.limit(actor_->max_force());
    steering = steering / actor_->mass();

    kglt::Vec3 new_velocity = actor_->velocity() + steering;
    new_velocity.limit(actor_->max_speed());
    return new_velocity;
}

kglt::Vec3 Boid::flee(const Vec3& target) const {
    kglt::Vec3 desired_velocity = (actor_->position() - target);
    kglt::Vec3 steering = desired_velocity - actor_->velocity();

    steering.limit(actor_->max_force());
    steering = steering / actor_->mass();

    kglt::Vec3 new_velocity = actor_->velocity() + steering;
    new_velocity.limit(actor_->max_speed());
    return new_velocity;
}

void Boid::follow(Path path) {
    assert(actor_);

    path_ = path;
    current_node_ = 0;


    int32_t path_length = path_.length();
    if(path_length > 0) {
        //Move the actor directly to the first waypoint
        actor_->actor()->set_absolute_position(path_.point(0));
    }
}

kglt::Vec3 Boid::steer_to_path() {
    if(path_.empty() || current_node_ >= path_.length()) {
        return kglt::Vec3();
    }

    kglt::Vec3 target = path_.point(current_node_);
    if(kglt::Vec3::distance(actor_->position(), target) < path_.radius()) {
        current_node_ += 1;
        if(path_.cyclic() && current_node_ == path_.length()) {
            current_node_ = 0;
        }
    }

    return seek(target, path_.radius());
}

void Boid::update_debug_mesh() const {
    Stage* stage = actor_->stage();
    assert(stage);

    auto mesh = stage->mesh(debug_mesh_);

    auto& vd = mesh->submesh(normal_points_mesh_).vertex_data();
    auto& id = mesh->submesh(normal_points_mesh_).index_data();

    vd.clear();
    id.clear();

    vd.move_to_start();

    int i = 0;
    for(kglt::Vec3 p: normal_points_) {
        vd.position(p);
        vd.diffuse(kglt::Colour::red);
        vd.tex_coord0(kglt::Vec2());
        vd.tex_coord1(kglt::Vec2());
        vd.normal(kglt::Vec3());
        vd.move_next();

        id.index(i++);
    }

    id.done();
    vd.done();
}

void Boid::enable_debug(bool value) {
    assert(actor_);
    Stage* stage = actor_->stage();
    assert(stage);

    if(value) {
        if(!debug_mesh_) {
            debug_mesh_ = stage->new_mesh();
        }

        auto mesh = stage->mesh(debug_mesh_);
        mesh->clear();

        auto smi = mesh->new_submesh(kglt::MaterialID(), MESH_ARRANGEMENT_LINE_STRIP, false);
        normal_points_mesh_ = mesh->new_submesh(kglt::MaterialID(), MESH_ARRANGEMENT_POINTS, false);

        {
            auto mat = stage->material(mesh->submesh(normal_points_mesh_).material_id());
            mat->technique().pass(0).set_point_size(5);
        }

        for(uint32_t i = 0; i < path_.length(); ++i) {
            mesh->submesh(smi).vertex_data().position(path_.point(i));
            mesh->submesh(smi).vertex_data().diffuse(kglt::Colour::blue);
            mesh->submesh(smi).vertex_data().tex_coord0(kglt::Vec2());
            mesh->submesh(smi).vertex_data().tex_coord1(kglt::Vec2());
            mesh->submesh(smi).vertex_data().normal(kglt::Vec3());
            mesh->submesh(smi).vertex_data().move_next();

            mesh->submesh(smi).index_data().index(i);
        }

        mesh->submesh(smi).vertex_data().done();
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


}
}
