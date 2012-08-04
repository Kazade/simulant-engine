#include <boost/format.hpp>

#include "kazbase/logging/logging.h"
#include "background.h"
#include "shortcuts.h"
#include "procedural/mesh.h"
#include "renderer.h"

namespace kglt {

BackgroundLayer::BackgroundLayer(Background &bg, const std::string& image_path):
    background_(bg),
    texture_id_(NullTextureID),
    width_(0),
    height_(0) {

    //Set the background as the parent
    set_parent(&background_);

    texture_id_ = kglt::create_texture_from_file(background().scene().window(), image_path);

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
    mesh_id_ = scene().new_mesh();

    //Set the mesh's parent as this
    Mesh& mesh = scene().mesh(mesh_id_);
    mesh.set_parent(this);
    kglt::procedural::mesh::rectangle(mesh, width(), height());
    mesh.apply_texture(texture_id_);

    //Disable depth testing stuff
    mesh.enable_depth_test(false);
    mesh.enable_depth_writes(false);
}

void BackgroundLayer::scroll_x(double amount) {
    offset_x_ += amount;
    offset_x_ -= floor(offset_x_); //Only leave the remainder (we want 0.0 - 1.0)

    Mesh& mesh = scene().mesh(mesh_id_);

    //Reset the texture coordinates
    kglt::procedural::mesh::rectangle(mesh, width(), height());
    for(uint32_t i = 0; i < 3; ++i) {
        mesh.triangle(0).uv(i).x += offset_x_;
        mesh.triangle(1).uv(i).x += offset_x_;
    }
    mesh.invalidate();
}

void BackgroundLayer::scroll_y(double amount) {
    offset_y_ += amount;
    offset_y_ -= floor(offset_y_);

    //FIXME: this is so inefficient it makes me cry - need a way to just invalidate tex coords (custom shader to use offset perhaps)
    Mesh& mesh = scene().mesh(mesh_id_);

    //Reset the texture coordinates
    kglt::procedural::mesh::rectangle(mesh, width(), height());
    for(uint32_t i = 0; i < 3; ++i) {
        mesh.triangle(0).uv(i).y += offset_y_;
        mesh.triangle(1).uv(i).y += offset_y_;
    }
    mesh.invalidate();
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

Background::Background() {}

void Background::add_layer(const std::string &image_path) {
    std::tr1::shared_ptr<BackgroundLayer> new_layer(new BackgroundLayer(*this, image_path));
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
}

/*
void Background::pre_visit(ObjectVisitor& visitor) {
    /**
     * FIXME:
     * This is a bit hacky, I should come up with something better. Basically, most elements in the scene
     * are rendered by the active camera. The camera controls the projection matrix and the viewing frustum
     * but in the case of a background I need to override the projection. Which I've done manually here.
     *
     * Perhaps a better way is to render the background using active_camera().frustum().far_corners() as the bounds,
     * and then use the texture coordinates of the background mesh to manipulate the visible area. That would
     * avoid this hack. The mesh would need to be rescaled each frame.

    if(Renderer* renderer = dynamic_cast<Renderer*>(&visitor)) {
        tmp_projection_ = renderer->projection_matrix(); //Store the projection matrix before

        kmMat4 new_proj;
        kmMat4OrthographicProjection(&new_proj, -visible_x_ / 2.0, visible_x_ / 2.0, -visible_y_ / 2.0, visible_y_ / 2.0, -1.0, 1.0);
        renderer->set_projection_matrix(new_proj);
    }
}

void Background::post_visit(ObjectVisitor& visitor) {
    if(Renderer* renderer = dynamic_cast<Renderer*>(&visitor)) {
        //Restore the stored projection
        renderer->set_projection_matrix(tmp_projection_);
    }
}*/

}
