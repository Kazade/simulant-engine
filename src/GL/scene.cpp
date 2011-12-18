
#include "scene.h"
#include "renderer.h"

namespace GL {

MeshID Scene::new_mesh() {
    static MeshID counter = 0;
    MeshID id = ++counter;

    meshes_.insert(std::make_pair(id, Mesh::ptr(new Mesh)));

    Mesh& mesh = *meshes_[id];
    mesh.set_parent(this);

    return id;
}

Mesh& Scene::mesh(MeshID m) {
    //FIXME: Assert contains
    return *meshes_[m];
}

TextureID Scene::new_texture() {
    static TextureID counter = 0;
    TextureID id = ++counter;
    Texture& tex = textures_[id];
    return id;
}

Texture& Scene::texture(TextureID t) {
    //FIXMe: assert this
    return textures_[t];
}

CameraID Scene::new_camera() {
    static CameraID counter = 0;
    CameraID id = ++counter;

    cameras_.insert(std::make_pair(id, Camera::ptr(new Camera)));

    Camera& cam = *cameras_[id];
    cam.set_parent(this);

    //We always need a camera, so if this is the
    //first one, then make it the current one
    if(cameras_.size() == 1) {
        current_camera_ = id;
    }

    return id;
}

Camera& Scene::camera(CameraID c) {
    //FIXME: Assert

    if(c == 0) {
        return *cameras_[current_camera_];
    }

    return *cameras_[c];
}

void Scene::render() {
    Renderer renderer(render_options);

    renderer.start_render(this);

    this->accept(renderer);

    renderer.finish_render();
}

}
