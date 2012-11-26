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
    texture_id_(0),
    material_id_(0),
    width_(0),
    height_(0),
    offset_x_(0),
    offset_y_(0) {

    texture_id_ = kglt::create_texture_from_file(background().scene().window(), image_path);
    material_id_ = kglt::create_material_from_texture(background().scene(), texture_id_);

    kglt::Texture& tex = background().scene().texture(texture_id_);
    width_ = tex.width();
    height_ = tex.height();

    //If there are already layers, make sure this image is the correct size
    if(background().layer_count() > 0) {
        uint32_t width = background().layer(0).width();
        uint32_t height = background().layer(0).height();

        if(width_ != width || height_ != height) {
            //FIXME: Should we clean up the texture here? I guess so...
            //background().scene().delete_texture(texture_id_);
            throw std::logic_error((boost::format("Invalid image size, layer already exists with dimensions: %d, %d") % width % height).str());
        }
    }

    //Create the mesh for this background layer
    mesh_id_ = background().scene().new_mesh();
    Mesh& mesh = background().scene().mesh(mesh_id_);
    kglt::procedural::mesh::rectangle(mesh, width(), height(), width() / 2.0, height() / 2.0);
    mesh.submesh(mesh.submesh_ids()[0]).set_material(material_id_);

    //Create the entity for this background
    entity_id_ = background().scene().new_entity();
    Entity& entity = background().scene().entity(entity_id_);
    entity.set_mesh(mesh.id());

    //Set the mesh's scene group to that of the background
    entity.scene_group = background().scene().scene_group(background().scene_group());

    L_WARN("Must disable depth writes on the background material");
    //Disable depth testing stuff
//    mesh.enable_depth_test(false);
//    mesh.enable_depth_writes(false);
}

void BackgroundLayer::scroll_x(double amount) {
    offset_x_ += amount;
    offset_x_ -= floor(offset_x_); //Only leave the remainder (we want 0.0 - 1.0)

    L_ERROR("Background scrolling disabled until I write a decent way of transforming texture coordinates");

/*
    Mesh& mesh = background().scene().mesh(mesh_id_);

    //Reset the texture coordinates
    kglt::procedural::mesh::rectangle(mesh, width(), height());
    for(uint32_t i = 0; i < 3; ++i) {
        mesh.triangle(0).uv(i).x += offset_x_;
        mesh.triangle(1).uv(i).x += offset_x_;
    }
    mesh.invalidate();*/
}

void BackgroundLayer::scroll_y(double amount) {
    offset_y_ += amount;
    offset_y_ -= floor(offset_y_);

    L_ERROR("Background scrolling disabled until I write a decent way of transforming texture coordinates");
/*
    //FIXME: this is so inefficient it makes me cry - need a way to just invalidate tex coords (custom shader to use offset perhaps)
    Mesh& mesh = background().scene().mesh(mesh_id_);

    //Reset the texture coordinates
    kglt::procedural::mesh::rectangle(mesh, width(), height());
    for(uint32_t i = 0; i < 3; ++i) {
        mesh.triangle(0).uv(i).y += offset_y_;
        mesh.triangle(1).uv(i).y += offset_y_;
    }
    mesh.invalidate();
*/
}

BackgroundLayer::~BackgroundLayer() {
    try {
        if(texture_id_) {
            background().scene().delete_texture(texture_id_);
        }

        if(material_id_) {
            background().scene().delete_material(material_id_);
        }

        if(mesh_id_) {
            background().scene().delete_mesh(mesh_id_);
        }

        if(entity_id_) {
            background().scene().delete_entity(entity_id_);
        }
    } catch (...) {
        //Whatever.. we tried. We gotta catch this (destructors can't throw)
        L_WARN("Didn't successfully delete background texture - scene was probably destroyed already");
    }
}

Background::Background(Scene& scene, ViewportID viewport):
    scene_(scene),
    viewport_(viewport) {

    background_sg_ = scene.new_scene_group(); //Create a SG for the background
    ortho_camera_ = scene.new_camera(); //Create an ortho cam

    //Add a pass for this background
    //FIXME: priority = -1000
    scene_.pipeline().add_pass(background_sg_, TextureID(), ortho_camera_, viewport_);
}

void Background::add_layer(const std::string &image_path) {
    BackgroundLayer::ptr new_layer(new BackgroundLayer(*this, image_path));
    layers_.push_back(new_layer);

    if(layer_count() == 1) {
        set_visible_dimensions(new_layer->width(), new_layer->height());
    }
}

void Background::set_visible_dimensions(double width, double height) {
    if(!layer_count()) {
        throw std::logic_error("You must add at least a single layer before setting the visible dimensions");
    }

    if(width > layer(0).width() || height > layer(0).height()) {
        throw std::logic_error("You cannot set the visible area larger than the layer size");
    }

    visible_x_ = width;
    visible_y_ = height;

    //Set the camera to the visible dimensions
    scene().camera(ortho_camera_).set_orthographic_projection(0, visible_x_, 0, visible_y_);
}

} //extra
}
