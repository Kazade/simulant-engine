#include "kglt/kglt.h"

using namespace kglt;

int main(int argc, char* argv[]) {
    Window::ptr window = Window::create();
    Scene& scene = window->scene();

    SceneGroup& post_process = return_new_scene_group(scene); //Create a post-process scene group

    Mesh& cube = return_new_mesh(scene); //Create a cube
    procedural::mesh::cube(cube);

    //Create a fullscreen quad, by attaching it to the post-process SG
    Mesh& fullscreen_quad = return_new_mesh(scene);
    procedural::mesh::rectangle(1.0, 1.0);
    fullscreen_quad.scene_group = post_process;

    //Create a camera with an orthographic projection so that the quad
    //renders fullscreen
    Camera& ortho_cam = return_new_camera(scene);
    ortho_cam.set_orthographic_projection(0, 1, 0, 1);

    //Create a target texture for the rendering
    Texture& target = return_new_texture(scene);
    target.resize(window->width(), window->height());

    //Create a material for our fullscreen quad
    Material& material = scene.new_material(scene.default_material()); //Clone the default material
    material.technique().pass(0).set_texture_unit(0, target); //Set the texture for the material to the target
    fullscreen_quad.apply_material(material);

    scene.pipeline().remove_all_passes(); //We want to construct our own pipeline
    scene.pipeline().add_pass(scene.scene_group().id(), target, ortho_cam.id()); //Render the scene to the texture
    scene.pipeline().add_pass(post_process.id()); //Render the fullscreen quad to the framebuffer

    while(window.update()) {}

    return 0;
}
