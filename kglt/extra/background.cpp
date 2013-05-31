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
#include "../stage.h"
#include "../loader.h"

namespace kglt {
namespace extra {

BackgroundLayer::BackgroundLayer(Background &bg, const std::string& image_path):
    background_(bg),
    texture_id_(0) {

    Stage& stage = background().stage();

    texture_id_ = kglt::create_texture_from_file(stage, image_path);
    pass_id_ = background().layer_count();

    kglt::MaterialPtr mat = stage.material(background().material_id()).lock();

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
    MaterialPtr mat = stage.material(background().material_id()).lock();
    mat->technique().pass(pass_id_).texture_unit(0).scroll_x(amount);
}

void BackgroundLayer::scroll_y(double amount) {
    /*
     *  Scrolls the background layer on the y-axis by 'amount'.
     *  This is simply a thunk to manipulate the underlyng texture matrix
     */
    Stage& stage = background().stage();
    MaterialPtr mat = stage.material(background().material_id()).lock();
    mat->technique().pass(pass_id_).texture_unit(0).scroll_y(amount);
}

BackgroundLayer::~BackgroundLayer() {
    Stage& stage = background().stage();
    kglt::MaterialPtr mat = stage.material(background().material_id()).lock();

    //Unset the texture on the material (decrementing the ref-count)
    mat->technique().pass(pass_id_).set_texture_unit(0, TextureID());
}

Background::Background(Scene& scene, ViewportID viewport, BGResizeStyle style):
    stage_(scene.stage(scene.new_stage(PARTITIONER_NULL))),
    viewport_(viewport),
    style_(style),
    entity_(nullptr) {

    ortho_camera_ = stage_.new_camera(); //Create an ortho cam

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
    stage_.camera(ortho_camera_).set_orthographic_projection(
        0,
        width,
        0,
        height,
        -1.0, 1.0
    );

    //Add a pass for this background
    //FIXME: priority = -1000
    scene.pipeline().add_stage(stage_.id(), ortho_camera_, viewport_, TextureID(), -100);

    entity_ = &stage_.entity(stage_.new_entity(stage_.new_mesh()));

    MaterialPtr mat = stage_.material(stage_.new_material()).lock();

    //Load the background material
    scene.window().loader_for("kglt/materials/background.kglm")->into(*mat);
    material_id_ = mat->id();

    SubMeshIndex index = kglt::procedural::mesh::rectangle(
        entity_->mesh().lock(),
        1,
        1,
        0.5,
        0.5
    );

    entity_->subentity(index).override_material_id(material_id_);
}

Background::~Background() {
    try {
        if(entity_) {
            stage_.delete_entity(entity_->id());
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
