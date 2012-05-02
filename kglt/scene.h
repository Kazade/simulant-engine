#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <boost/any.hpp>
#include <stdexcept>
#include <map>

#include "kazbase/list_utils.h"

#include "object.h"
#include "mesh.h"
#include "camera.h"
#include "renderer.h"
#include "texture.h"
#include "shader.h"
#include "viewport.h"
#include "sprite.h"

namespace kglt {

class Window;

class Scene : public Object, public Loadable {
public:
    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the scene");
    }

    Scene(Window* window):
        window_(window),
        viewport_(this) {
        new_camera(); //Create a default camera

        /*
            TODO: Load the default shader which simply renders textured
            polygons like the fixed function.
        */

        //Set up the default render options
        render_options.wireframe_enabled = false;
        render_options.texture_enabled = true;
        render_options.backface_culling_enabled = true;
        render_options.point_size = 1;
    }

    MeshID new_mesh();
    CameraID new_camera();
    TextureID new_texture();
    ShaderID new_shader();
    SpriteID new_sprite();

    Mesh& mesh(MeshID m);
    Camera& camera(CameraID c = DefaultCameraID);
    Texture& texture(TextureID t);
    ShaderProgram& shader(ShaderID s = NullShaderID);
    Sprite& sprite(SpriteID s);

    void init();
    void render();
    void update(double dt);

    void accept(ObjectVisitor& visitor) {
        for(Object* child: children_) {
            child->accept(visitor);
        }

        visitor.visit(this);
    }

    RenderOptions render_options;

    Window& window() { return *window_; }
    Viewport& viewport() { return viewport_; }

    void set_extra_data(const std::string& name, boost::any extra_scene_data) { extra_scene_data_[name] = extra_scene_data; }

    template<typename T>
    T extra_data_as(const std::string& name) {
        if(!container::contains(extra_scene_data_, name)) {
            throw std::logic_error("No such extra data attached to the scene: " + name);
        }
        return boost::any_cast<T>(extra_scene_data_[name]);
    }

private:
    std::map<MeshID, Mesh::ptr> meshes_;
    std::map<CameraID, Camera::ptr> cameras_;
    std::map<TextureID, Texture> textures_;
    std::map<ShaderID, ShaderProgram> shaders_;
    std::map<SpriteID, Sprite::ptr> sprites_;

    CameraID current_camera_;
    Window* window_;
    Viewport viewport_;

    std::map<std::string, boost::any> extra_scene_data_;
};

}

#endif // SCENE_H_INCLUDED
