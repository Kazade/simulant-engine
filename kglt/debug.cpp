#include "stage.h"
#include "debug.h"
#include "actor.h"
#include "window.h"

namespace kglt {

Debug::Debug(Stage &stage):
    stage_(stage) {

}

void Debug::update() {
    double dt = stage_.window().delta_time();

    std::vector<DebugElement> to_keep;

    for(auto elem: elements_) {
        elem.time_since_created += dt;
        if(elem.time_since_created >= elem.duration) {
            auto mesh = stage_.mesh(mesh_);
            mesh->delete_submesh(elem.submesh);
        } else {
            to_keep.push_back(elem);
        }
    }

    elements_ = to_keep;
}

bool Debug::init() {
    mesh_ = stage_.new_mesh();
    actor_ = stage_.new_actor_with_mesh(mesh_);

    //Don't GC the material, if there are no debug lines then it won't be attached to the mesh
    material_ = stage_.new_material_from_file("kglt/materials/diffuse_render.kglm", /*garbage_collect=*/false);

    //Connect regular updates so we can remove debug lines after their duration
    stage_.window().signal_frame_finished().connect(
        std::bind(&Debug::update, this)
    );

    return true;
}

void Debug::draw_line(const Vec3 &start, const Vec3 &end, const Colour &colour, double duration, bool depth_test) {

    auto mesh = stage_.mesh(mesh_);

    DebugElement element;
    element.submesh = mesh->new_submesh(material_, MESH_ARRANGEMENT_LINE_STRIP, /*shares_vertices=*/ false);
    element.colour = colour;
    element.duration = duration;
    element.depth_test = depth_test;
    element.type = DebugElementType::DET_LINE;

    auto& submesh = mesh->submesh(element.submesh);

    submesh.vertex_data().move_to_start();
    submesh.vertex_data().position(start);
    submesh.vertex_data().diffuse(colour);
    submesh.vertex_data().move_next();

    submesh.vertex_data().position(end);
    submesh.vertex_data().diffuse(colour);
    submesh.vertex_data().done();

    submesh.index_data().index(0);
    submesh.index_data().index(1);
    submesh.index_data().done();

    elements_.push_back(element);
}

void Debug::draw_ray(const Vec3 &start, const Vec3 &dir, const Colour &colour, double duration, bool depth_test) {
    draw_line(start, start+dir, colour, duration, depth_test);
}

void Debug::draw_point(const Vec3 &position, const Colour &colour, double duration, bool depth_test) {

}

}
