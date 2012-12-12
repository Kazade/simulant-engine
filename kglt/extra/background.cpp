#include <boost/format.hpp>

#include "kazbase/logging/logging.h"
#include "background.h"

#include "../shortcuts.h"
#include "../procedural/mesh.h"
#include "../renderer.h"
#include "../scene.h"

namespace kglt {
namespace extra {

BackgroundLayer::BackgroundLayer(Background &bg, const std::string& image_path):
    background_(bg),
    texture_id_(0) {

    texture_id_ = kglt::create_texture_from_file(background().scene().window(), image_path);
    texture_unit_ = background().layer_count();

    kglt::Material& mat = background().scene().material(background().material());
    mat.technique().pass(0).set_texture_unit(texture_unit_, texture_id_);
}

void BackgroundLayer::scroll_x(double amount) {
    Material& mat = background().scene().material(background().material());
    mat.technique().pass(0).texture_unit(texture_unit_).scroll_x(amount);
}

void BackgroundLayer::scroll_y(double amount) {
    Material& mat = background().scene().material(background().material());
    mat.technique().pass(0).texture_unit(texture_unit_).scroll_y(amount);
}

BackgroundLayer::~BackgroundLayer() {
    try {
        if(texture_id_) {
            background().scene().delete_texture(texture_id_);
        }
    } catch (...) {
        //Whatever.. we tried. We gotta catch this (destructors can't throw)
        L_WARN("Didn't successfully delete background texture - scene was probably destroyed already");
    }
}

Background::Background(Scene& scene, ViewportID viewport, BGResizeStyle style):
    scene_(scene),
    viewport_(viewport),
    style_(style) {

    background_sg_ = scene.new_scene_group(); //Create a SG for the background
    ortho_camera_ = scene.new_camera(); //Create an ortho cam

    float width, height;
    if(style_ == BG_RESIZE_ZOOM) {
        Viewport& vp = scene.window().viewport(viewport);

        if(vp.width() > vp.height()) {
            width = 1.0;
            height = float(vp.height()) / float(vp.width());
        } else {
            height = 1.0;
            width = float(vp.width()) / float(vp.height());
        }
    } else {
        width = 1.0;
        height = 1.0;
    }


    //Set the camera to the visible dimensions
    scene.camera(ortho_camera_).set_orthographic_projection(
        0,
        width,
        0,
        height,
        -1.0, 1.0
    );

    //Add a pass for this background
    //FIXME: priority = -1000
    scene_.pipeline().add_pass(background_sg_, TextureID(), ortho_camera_, viewport_);

    mesh_id_ = scene.new_mesh();
    Mesh& mesh = scene.mesh(mesh_id_);
    material_id_ = scene.new_material(scene.default_material());

    SubMeshIndex index = kglt::procedural::mesh::rectangle(
        mesh,
        1,
        1,
        0.5,
        0.5
    );

    mesh.submesh(index).set_material(material_id_);

    L_WARN("Must disable depth writes on the background material");
    //Disable depth testing stuff
//    mesh.enable_depth_test(false);
//    mesh.enable_depth_writes(false);

    //Create the entity for this background
    entity_id_ = scene.new_entity();
    Entity& entity = scene.entity(entity_id_);
    entity.set_mesh(mesh.id());

    //Set the mesh's scene group to that of the background
    entity.scene_group = scene.scene_group(scene_group());
}

Background::~Background() {
    try {
        if(mesh_id_) {
            scene().delete_mesh(mesh_id_);
        }

        if(entity_id_) {
            scene().delete_entity(entity_id_);
        }
    } catch(...) {
        L_WARN("Error while destroying background");
    }
}

void Background::add_layer(const std::string &image_path) {
    BackgroundLayer::ptr new_layer(new BackgroundLayer(*this, image_path));
    layers_.push_back(new_layer);
}


} //extra
}
