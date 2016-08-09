#include "rigid_body.h"
#include "../object.h"
#include "../actor.h"
#include "../stage.h"

namespace kglt {
namespace controllers {

int overlap_shapes(void* ctx, ym_vector<ym_vec2i>* overlaps) {
    yb_scene* scene_bvh = (yb_scene*)ctx;
    return yb_overlap_shape_bounds(scene_bvh, true, overlaps);
}

bool overlap_shape(void* ctx, int sid, const ym_vec3f& pt, float max_dist,
                   float* dist, int* eid, ym_vec2f* euv) {
    yb_scene* scene_bvh = (yb_scene*)ctx;
    return yb_overlap_first(scene_bvh, pt, max_dist, dist, &sid, eid, euv, sid);
}

void overlap_refit(void* ctx, const ym_frame3f* xforms) {
    yb_scene* scene_bvh = (yb_scene*)ctx;
    yb_refit_scene_bvh(scene_bvh, (const ym_affine3f*)xforms);
}

RigidBodySimulation::RigidBodySimulation(uint32_t max_bodies):
    max_bodies_(max_bodies) {

}

int ysr_push_body(ysr_scene* scene) {
    scene->bodies.push_back(ysr__body());
    scene->frame.push_back(ym_frame3f());
    scene->lin_vel.push_back(ym_vec3f());
    scene->ang_vel.push_back(ym_vec3f());

    return (scene->bodies.size() - 1);
}

void ysr_erase_body(ysr_scene* scene, int bid) {
    // HALP!
}

int yb_push_shape(yb_scene* scene, const ym_affine3f& xform,
                   int nelems, const int* elem, int etype, int nverts,
                   const ym_vec3f* pos, const float* radius) {
    /*
     * The yocto BVH stuff doesn't allow dynamically adding shapes,
     * this hacks it in
     */

    scene->shapes.push_back(yb_shape());
    scene->xforms.push_back(ym_identity_affine3f);
    scene->inv_xforms.push_back(ym_identity_affine3f);

    int sid = (scene->shapes.size() - 1);

    yb_set_shape(scene, sid, xform, nelems, elem, etype, nverts, pos, radius);
    yb_build_bvh(scene, 0);
    return sid;
}

bool RigidBodySimulation::init() {
    scene_ = ysr_init_scene(0);
    bvh_scene_ = yb_init_scene(0);

    // setup collisions
    ysr_set_collision(scene_, bvh_scene_, overlap_shapes, overlap_shape, overlap_refit);

    return true;
}

void RigidBodySimulation::cleanup() {
    ysr_free_scene(scene_);
}

Mat3 from_ym_mat3f(const ym_mat3f& rhs) {
    Mat3 ret;
    for(uint32_t i = 0; i < 9; ++i) {
        ret.mat[i] = rhs.data()[i];
    }
    return ret;
}

void RigidBodySimulation::step(double dt) {

    // Apply force and torque:
    for(auto& p: bodies_) {
        RigidBody* body = dynamic_cast<RigidBody*>(p.first);
        if(!body) {
            continue;
        }

        uint32_t i = p.second;

        float inv_mass = scene_->bodies[i].mass_inv;
        Vec3 additional_linear = body->force_ * (inv_mass * dt);

        scene_->lin_vel[i].v[0] += additional_linear.x;
        scene_->lin_vel[i].v[1] += additional_linear.y;
        scene_->lin_vel[i].v[2] += additional_linear.z;

        Mat3 inv_inertia = from_ym_mat3f(scene_->bodies[i].inertia_inv);

        Vec3 additional_angular;
        kmVec3MultiplyMat3(&additional_angular, &body->torque_, &inv_inertia);
        additional_angular *= dt;

        scene_->ang_vel[i].v[0] += additional_angular.x;
        scene_->ang_vel[i].v[1] += additional_angular.y;
        scene_->ang_vel[i].v[2] += additional_angular.z;

        body->force_ = Vec3();
        body->torque_ = Vec3();
    }

    ysr_advance(scene_, dt);
}

Vec3 RigidBodySimulation::intersect_ray(const Vec3& start, const Vec3& direction) {
    ym_ray3f ray;
    ray.o[0] = start.x;
    ray.o[1] = start.y;
    ray.o[2] = start.z;

    ray.d[0] = direction.x;
    ray.d[1] = direction.y;
    ray.d[2] = direction.z;

    float dist;
    int sid, eid;
    ym_vec2f euv;

    yb_intersect_first(
        bvh_scene_,
        ray,
        &dist, &sid, &eid, &euv
    );

    return Vec3(start) + (Vec3(direction).normalized() * dist);
}

uint32_t RigidBodySimulation::acquire_body(impl::Body *body) {
    uint32_t i = ysr_push_body(scene_);
    bodies_[body] = i;
    scene_->bodies[i].mass = 1.0; // Enable the body in the simulation   
    return i;
}

void RigidBodySimulation::release_body(impl::Body *body) {
    auto i = bodies_.at(body);
    scene_->bodies[i].mass = 0;
    bodies_.erase(body);
    ysr_erase_body(scene_, i);
}

std::pair<Vec3, Quaternion> RigidBodySimulation::body_transform(impl::Body *body) {
    ym_frame3f frame = ysr_get_transform(scene_, bodies_.at(body));

    return std::make_pair(
        Vec3(frame.t[0], frame.t[1], frame.t[2]),
        Quaternion(Mat3(frame.m.data()))
    );
}

void RigidBodySimulation::set_body_transform(impl::Body* body, const Vec3& position, const Quaternion& rotation) {
    ym_frame3f frame;
    frame.t[0] = position.x;
    frame.t[1] = position.y;
    frame.t[2] = position.z;

    Mat3 rot;
    kmMat3FromRotationQuaternion(&rot, &rotation);
    for(uint32_t i = 0; i < 9; ++i) { frame.m.data()[i] = rot.mat[i]; }

    ysr_set_transform(scene_, bodies_.at(body), frame);
}

ysr__body* RigidBodySimulation::get_ysr_body(impl::Body* body) {
    return &scene_->bodies[bodies_.at(body)];
}

RigidBody::RigidBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Body(object, simulation) {

}

RigidBody::~RigidBody() {

}

void RigidBody::add_force(const Vec3 &force) {
    force_ += force;
}

void RigidBody::add_force_at_position(const Vec3& force, const Vec3& position) {
    // FIXME: Should use shape position, probably...
    Vec3 com = simulation_->body_transform(this).first;

    Vec3 centre_dir = position - com;
    Vec3 torque_force = centre_dir.cross(force);

    add_force(force);
    add_torque(torque_force);
}

void RigidBody::add_torque(const Vec3& torque) {
    torque_ += torque;
}

StaticBody::StaticBody(Controllable* object, RigidBodySimulation::ptr simulation):
    Body(object, simulation) {

    // Make sure we set the mass to zero so we don't simulate this body
    auto body = simulation->get_ysr_body(this);
    body->mass = 0;
}

StaticBody::~StaticBody() {

}


namespace impl {

Body::Body(Controllable* object, RigidBodySimulation::ptr simulation, ColliderType collider_type):
    Controller("rigid-body"),
    simulation_(simulation) {

    object_ = dynamic_cast<MoveableObject*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }

    body_id_ = simulation->acquire_body(this);

    build_collider(collider_type);
}

Body::~Body() {
    simulation_->release_body(this);
}

void Body::move_to(const Vec3& position) {
    auto xform = simulation_->body_transform(this);
    simulation_->set_body_transform(
        this,
        position,
        xform.second
    );
}

void Body::do_post_update(double dt) {
    auto xform = simulation_->body_transform(this);
    object_->set_absolute_position(xform.first);
    object_->set_absolute_rotation(xform.second);
}

void Body::build_collider(ColliderType collider) {
    if(collider == COLLIDER_TYPE_NONE) return;
    return;

    if(collider == COLLIDER_TYPE_MESH) {
        // Must be an actor (particle systems, lights, cameras etc. don't have meshes)
        // FIXME: Could be a geom in future, but geoms are broken :/
        Actor* actor = dynamic_cast<Actor*>(object_);
        if(!actor || !actor->mesh_id()) {
            throw std::runtime_error("Tried to add a mesh collider to an object which isn't an actor, or doesn't have a mesh");
        }

        auto mesh = actor->stage->assets->mesh(actor->mesh_id());

        /*
         * So ysr_compute_moments seems partially implemented, and currently only accepts triangles.
         * We must build a list of triangles, if any of the submeshes *aren't* in triangle format then we bail */

        auto& indexes = shape.indexes;
        auto& vertices = shape.vertices;

        indexes.clear();
        vertices.clear();

        mesh->each([&](SubMesh* submesh) {
            if(submesh->arrangement() != MESH_ARRANGEMENT_TRIANGLES) {
                throw std::runtime_error("Only triangle meshes are currently supported");
            }

            auto* index_data = submesh->index_data.get();
            auto* vertex_data = submesh->vertex_data.get();
            for(uint32_t i = 0; i < index_data->count(); i+=3) {
                Vec3 v = vertex_data->position_at<Vec3>(index_data->at(i));
                ym_vec3f yv;
                yv[0] = v.x;
                yv[1] = v.y;
                yv[2] = v.z;
                vertices.push_back(yv);
                indexes.push_back(shape.vertices.size() - 1);
            }
        });

        auto body = simulation_->get_ysr_body(this);

        float mass = body->mass;
        ym_mat3f inertia = ym_identity_mat3f;

        if(mass > 0) {
            // If this isn't a static body, calculate inertia etc.
            float volume;
            ym_vec3f center;

            ysr_compute_moments(
                indexes.size() / 3,
                &indexes[0],
                shape.element_type, // Apparently etype is unused here
                vertices.size(),
                &vertices[0],
                &volume, &center, &inertia
            );

            for(auto& v: vertices) { v -= center; };
            mass *= volume;
        }

        ysr_set_body(
            simulation_->scene_,
            simulation_->bodies_.at(this),
            ym_identity_frame3f,
            mass,
            inertia,
            indexes.size() / 3,
            &indexes[0],
            shape.element_type,
            vertices.size(),
            &vertices[0]
        );

        int sid = yb_push_shape(
            simulation_->bvh_scene_,
            ym_identity_affine3f,
            shape.indexes.size() / 3,
            &shape.indexes[0],
            shape.element_type,
            shape.vertices.size(),
            &shape.vertices[0],
            nullptr
        );

        assert(sid == simulation_->bodies_.at(this));
    }
}

}



}
}
