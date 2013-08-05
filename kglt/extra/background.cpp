#include <boost/format.hpp>

#include "../kazbase/logging.h"
#include "background.h"

#include "../actor.h"
#include "../render_sequence.h"
#include "../camera.h"
#include "../shortcuts.h"
#include "../procedural/mesh.h"
#include "../renderer.h"
#include "../scene.h"
#include "../stage.h"
#include "../loader.h"

namespace kglt {
namespace extra {

BackgroundLayer::BackgroundLayer(Background &bg, const std::string& image_path):
    background_(bg),
    texture_id_(0) {

    Stage& stage = background().stage();

    texture_id_ = stage.new_texture_from_file(image_path);
    pass_id_ = background().layer_count();

    auto mat = stage.material(background().material_id());

    if(pass_id_ >= mat->technique().pass_count()) {
        //Duplicate the first pass to create this one
        mat->technique().new_pass(mat->technique().pass(0).shader_id());

        //All passes except the first should have alpha blending enabled
        mat->technique().pass(pass_id_).set_blending(BLEND_ALPHA);
        mat->technique().pass(pass_id_).set_depth_test_enabled(false);
        mat->technique().pass(pass_id_).set_depth_write_enabled(false);
    }

    //Finally set the texture on the new pass
    mat->technique().pass(pass_id_).set_texture_unit(0, texture_id_);
}

void BackgroundLayer::scroll_x(double amount) {
    /*
     *  Scrolls the background layer on the x-axis by 'amount'.
     *  This is simply a thunk to manipulate the underlyng texture matrix
     */
    Stage& stage = background().stage();
    auto mat = stage.material(background().material_id());
    mat->technique().pass(pass_id_).texture_unit(0).scroll_x(amount);
}

void BackgroundLayer::scroll_y(double amount) {
    /*
     *  Scrolls the background layer on the y-axis by 'amount'.
     *  This is simply a thunk to manipulate the underlyng texture matrix
     */
    Stage& stage = background().stage();
    auto mat = stage.material(background().material_id());
    mat->technique().pass(pass_id_).texture_unit(0).scroll_y(amount);
}

BackgroundLayer::~BackgroundLayer() {    
    Stage& stage = background().stage();
    auto mat = stage.material(background().material_id());
    if(mat) {
        //Unset the texture on the material (decrementing the ref-count)
        mat->technique().pass(pass_id_).set_texture_unit(0, TextureID());
    }
}

Background::Background(Scene& scene, ViewportID viewport, BGResizeStyle style):
    stage_(scene.stage(scene.new_stage(PARTITIONER_NULL))),
    viewport_(viewport),
    style_(style) {

    ortho_camera_ = scene.camera_ref(scene.new_camera()); //Create an ortho cam

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
    ortho_camera_.lock()->set_orthographic_projection(
        0,
        width,
        0,
        height,
        -1.0, 1.0
    );

    //Add a pass for this background
    //FIXME: priority = -1000
    pipeline_ = scene.render_sequence().new_pipeline(
        stage_.id(),
        ortho_camera_.lock()->id(),
        viewport_,
        TextureID(),
        -100
    );

    actor_ = stage_.new_actor(stage_.new_mesh());

    material_ = stage_.new_material();

    //Load the background material
    scene.window().loader_for("kglt/materials/background.kglm")->into(stage_.material(material_));

    SubMeshIndex index = kglt::procedural::mesh::rectangle(
        stage().actor(actor_)->mesh().lock(),
        1,
        1,
        0.5,
        0.5
    );

    stage().actor(actor_)->set_mesh(stage().actor(actor_)->mesh_id()); //FIXME: This is a workaround
    stage().actor(actor_)->override_material_id(material_);
}

MaterialID Background::material_id() const {
    return material_;
}

Background::~Background() {
    try {
        if(actor_) {
            stage_.delete_actor(actor_);
        }

        CameraPtr camera = ortho_camera_.lock();
        if(camera) {
            stage_.scene().delete_camera(camera->id());
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
