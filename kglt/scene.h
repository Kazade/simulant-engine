#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <boost/any.hpp>
#include <boost/thread/mutex.hpp>

#include <stdexcept>
#include <map>

#include "kazbase/list_utils.h"

#include "types.h"
#include "object.h"
#include "mesh.h"
#include "camera.h"
#include "renderer.h"
#include "texture.h"
#include "shader.h"
#include "viewport.h"
#include "sprite.h"
#include "pass.h"

#include "rendering/generic_renderer.h"

namespace kglt {

class WindowBase;

class Scene : public Object, public Loadable {
public:
    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the scene");
    }

    Scene(WindowBase* window):
        window_(window) {
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
        
		/**
		 * Create the default pass, which uses a perspective projection and
		 * a fullscreen viewport
		 */
		 add_pass(Renderer::ptr(new GenericRenderer(*this)), VIEWPORT_TYPE_FULL);
		 pass().renderer().set_perspective_projection(45.0, 0.1, 1000.0);        
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

	std::pair<ShaderID, bool> find_shader(const std::string& name);

    void delete_mesh(MeshID mid);

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

    WindowBase& window() { return *window_; }

    void set_extra_data(const std::string& name, boost::any extra_scene_data) { extra_scene_data_[name] = extra_scene_data; }

    template<typename T>
    T extra_data_as(const std::string& name) {
        if(!container::contains(extra_scene_data_, name)) {
            throw std::logic_error("No such extra data attached to the scene: " + name);
        }
        return boost::any_cast<T>(extra_scene_data_[name]);
    }

	void add_pass(
		Renderer::ptr renderer, 
		ViewportType viewport=VIEWPORT_TYPE_FULL,
		CameraID camera_id = DefaultCameraID
	) {
		Pass p(this, renderer, viewport, camera_id);
		passes_.push_back(p);
	}
	
	void remove_all_passes() {
		passes_.clear();
	}
	
	Pass& pass(uint32_t index = 0) { return passes_.at(index); }
	
	
	MeshID _mesh_id_from_mesh_ptr(Mesh* mesh);
	
private:
    std::map<MeshID, Mesh::ptr> meshes_;
    std::map<CameraID, Camera::ptr> cameras_;
    std::map<TextureID, Texture> textures_;
    std::map<ShaderID, ShaderProgram::ptr> shaders_;
    std::map<SpriteID, Sprite::ptr> sprites_;

    CameraID current_camera_;
    WindowBase* window_;

    std::map<std::string, boost::any> extra_scene_data_;
    
    Texture null_texture_;
    
    std::vector<Pass> passes_;
    
    boost::mutex scene_lock_;
};

}

#endif // SCENE_H_INCLUDED
