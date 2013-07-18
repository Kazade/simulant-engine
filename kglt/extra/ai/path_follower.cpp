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

kglt::Vec3 PathFollower::force_to_apply(const Vec3 &velocity) const {
    /**
      This is loosely based on http://natureofcode.com/book/chapter-6-autonomous-agents/#chapter06_section8
      but with handles the case where the velocity is at right angles to the path. It does this by
      first checking the normal point is on the line, then moving the normal point very slightly
      along the path. This solves two problems:

      1. We stop considering the current path segment when we approach the end of it
      2. We'll never hit a path segment at a right angle and get stuck as the force
         to apply is the reverse of the velocity because the slight offset means that there
         will always be a bias towards the direction of the path.

      The other change that's been made is that when the object reaches the last point on the
      path, it will slow to a stop.
    */

    if(path_.empty()) {
        return kglt::Vec3(0, 0, 0);
    }

    kglt::Vec3 predict = velocity;
    predict.normalize();

    kglt::Vec3 pos = actor_->actor()->absolute_position();
    kglt::Vec3 predict_loc = pos + predict;

    kglt::Vec3 target;
    float closest = 100000;

    normal_points_.clear();
    for(int i = 0; i < path_.length() - 1; ++i) {
        kglt::Vec3 a = path_.point(i);
        kglt::Vec3 b = path_.point(i+1);
        kglt::Vec3 path_dir = b - a;
        path_dir.normalize();

        path_dir = path_dir * 0.0001;

        kglt::Vec3 normal_point = get_normal_point(predict_loc, a, b);

        if(!point_on_line(normal_point, a, b)) {
            normal_point = b;
        }

        normal_point = normal_point + path_dir;
        if(!point_on_line(normal_point, a, b)) {
            //We're near the end of the line, so don't consider
            //this path anymore, unless this is the last path segment
            if(i + 2 != path_.length() && !loop_)
                continue;
        }

        normal_points_.push_back(normal_point);

        float dist = (predict_loc - normal_point).length();
        std::cout << i << " - " << dist;
        std::cout << "(" << normal_point.x << "," << normal_point.y << "," << normal_point.z << ")" << std::endl;
        if(dist < closest) {
            target = normal_point;
            closest = dist;
        }
    }

    float distance = (target - predict_loc).length();

    update_debug_mesh();

    if(distance > path_.radius()) {
        return seek(target, velocity);
    }

    return velocity;
}

Vec3 PathFollower::seek(const kglt::Vec3& target, const kglt::Vec3& velocity) const {
    kglt::Vec3 desired = target - actor_->actor()->absolute_position();

    float d = desired.length();
    desired.normalize();

    float slow_down_radius = path_.radius();
    if(d < slow_down_radius) {
        //Slow down as we approach the target
        float m = map(d, 0, slow_down_radius, 0, max_speed_);
        desired = desired * m;
    } else {
        desired = desired * max_speed_;
    }

    kglt::Vec3 steer = desired - velocity;

    steer.limit(max_force_);

    return steer;
}

void PathFollower::follow(Path path, bool loop) {
    assert(actor_);

    path_ = path;
    loop_ = loop;

    //Move the actor directly to the first waypoint
    actor_->actor()->move_to(path_.point(0));
}

void PathFollower::update_debug_mesh() const {
    Stage* stage = actor_->stage();
    assert(stage);

    auto mesh = stage->mesh(debug_mesh_).lock();

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
