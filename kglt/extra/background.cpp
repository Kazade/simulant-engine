#include <boost/format.hpp>

#include "../kazbase/logging.h"
#include "background.h"

#include "../entity.h"
#include "../pipeline.h"
#include "../camera.h"
#include "../shortcuts.h"
#include "../procedural/mesh.h"
#include "../renderer.h"
#include "../scene.h"
#include "../subscene.h"
#include "../loader.h"

namespace kglt {
namespace extra {

BackgroundLayer::BackgroundLayer(Background &bg, const std::string& image_path):
    background_(bg),
    texture_id_(0) {

    SubScene& subscene = background().scene().subscene(background().subscene_id());

    texture_id_ = kglt::create_texture_from_file(subscene, image_path);
    pass_id_ = background().layer_count();

    kglt::Material& mat = subscene.material(background().material_id());

    if(pass_id_ >= mat.technique().pass_count()) {
        //Duplicate the first pass to create this one
        mat.technique().new_pass(mat.technique().pass(0).shader());

        //All passes except the first should have alpha blending enabled
        mat.technique().pass(pass_id_).set_blending(BLEND_ALPHA);
        mat.technique().pass(pass_id_).set_depth_test_enabled(false);
        mat.technique().pass(pass_id_).set_depth_write_enabled(false);
    }

    //Finally set the texture on the new pass
    mat.technique().pass(pass_id_).set_texture_unit(0, texture_id_);
}

void BackgroundLayer::scroll_x(double amount) {
    /*
     *  Scrolls the background layer on the x-axis by 'amount'.
     *  This is simply a thunk to manipulate the underlyng texture matrix
     */
    SubScene& subscene = background().scene().subscene(background().subscene_id());
    Material& mat = subscene.material(background().material_id());
    mat.technique().pass(pass_id_).texture_unit(0).scroll_x(amount);
}

void BackgroundLayer::scroll_y(double amount) {
    /*
     *  Scrolls the background layer on the y-axis by 'amount'.
     *  This is simply a thunk to manipulate the underlyng texture matrix
     */
    SubScene& subscene = background().scene().subscene(background().subscene_id());
    Material& mat = subscene.material(background().material_id());
    mat.technique().pass(pass_id_).texture_unit(0).scroll_y(amount);
}

BackgroundLayer::~BackgroundLayer() {
    try {
        if(texture_id_) {
            SubScene& subscene = background().scene().subscene(background().subscene_id());
            subscene.delete_texture(texture_id_);
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

    //Create a subscene for the background
    subscene_ = scene.new_subscene(PARTITIONER_NULL); //Always draw the background - don't cull it
    ortho_camera_ = scene.subscene(subscene_).new_camera(); //Create an ortho cam

    SubScene& subscene = scene.subscene(subscene_);

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
    subscene.camera(ortho_camera_).set_orthographic_projection(
        0,
        width,
        0,
        height,
        -1.0, 1.0
    );

    //Add a pass for this background
    //FIXME: priority = -1000
    scene_.pipeline().add_stage(subscene_, ortho_camera_, viewport_, TextureID(), -100);

    mesh_id_ = subscene.new_mesh();
    Mesh& mesh = subscene.mesh(mesh_id_);
    Material& mat = subscene.material(subscene.new_material());

    //Load the background material
    scene.window().loader_for("kglt/materials/background.kglm")->into(mat);
    material_id_ = mat.id();

    SubMeshIndex index = kglt::procedural::mesh::rectangle(
        mesh,
        1,
        1,
        0.5,
        0.5
    );

    mesh.submesh(index).set_material_id(material_id_);

    //Create the entity for this background
    entity_id_ = subscene.new_entity();
    Entity& entity = subscene.entity(entity_id_);
    entity.set_mesh(mesh.id());
}

Background::~Background() {
    try {
        if(mesh_id_) {
            scene().subscene(subscene_id()).delete_mesh(mesh_id_);
        }

        if(entity_id_) {
            scene().subscene(subscene_id()).delete_entity(entity_id_);
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
