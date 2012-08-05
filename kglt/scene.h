#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <sigc++/sigc++.h>
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
#include "background.h"
#include "font.h"
#include "text.h"

#include "ui.h"
#include "rendering/generic_renderer.h"

#include "generic/visitor.h"
#include "generic/manager.h"

namespace kglt {

class WindowBase;

class Scene :
    public Object,
    public Loadable,
    public generic::TemplatedManager<Mesh, MeshID>,
    public generic::TemplatedManager<Sprite, SpriteID>,
    public generic::TemplatedManager<Camera, CameraID>,
    public generic::TemplatedManager<Text, TextID>,
    public generic::TemplatedManager<ShaderProgram, ShaderID> {

public:
    VIS_DEFINE_VISITABLE();

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the scene");
    }

    Scene(WindowBase* window);

    MeshID new_mesh();
    CameraID new_camera();
    TextureID new_texture();
    ShaderID new_shader();
    SpriteID new_sprite();
    FontID new_font();
    TextID new_text();

    Mesh& mesh(MeshID m);
    Camera& camera(CameraID c = DefaultCameraID);
    Texture& texture(TextureID t);
    ShaderProgram& shader(ShaderID s);
    Sprite& sprite(SpriteID s);
    Font& font(FontID f);

    Camera& active_camera() { return camera(active_camera_); }
    void set_active_camera(CameraID cam) { active_camera_ = cam; }

    Text& text(TextID t);
    const Text& text(TextID t) const;

	std::pair<ShaderID, bool> find_shader(const std::string& name);

    void delete_mesh(MeshID mid);
    void delete_texture(TextureID tid);
    void delete_sprite(SpriteID sid);
    void delete_camera(CameraID cid);
    void delete_text(TextID tid);
    void delete_shader(ShaderID s);

    void init();
    void render();
    void update(double dt);

    RenderOptions render_options;

    WindowBase& window() { return *window_; }

	void add_pass(
		Renderer::ptr renderer, 
		ViewportType viewport=VIEWPORT_TYPE_FULL,
        CameraID camera_id=DefaultCameraID
	) {
        renderer->_initialize(*this); //Initialize the renderer if need be

		Pass p(this, renderer, viewport, camera_id);
		passes_.push_back(p);
	}
	
	void remove_all_passes() {
		passes_.clear();
	}
	
	Pass& pass(uint32_t index = 0) { return passes_.at(index); }
	
	
	MeshID _mesh_id_from_mesh_ptr(Mesh* mesh);
	
    Background& background() { return background_; }
    UI& ui() { return ui_interface_; }

    sigc::signal<void, Pass&>& signal_render_pass_started() { return signal_render_pass_started_; }
    sigc::signal<void, Pass&>& signal_render_pass_finished() { return signal_render_pass_finished_; }

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(this);
        obj._initialize(*this);
    }

    void post_create_shader_callback(ShaderProgram& obj, ShaderID id) {
        shader_lookup_[obj.name()] = id;
    }

private:
    std::map<TextureID, Texture> textures_;
    std::map<FontID, Font::ptr> fonts_;

    std::map<std::string, ShaderID> shader_lookup_;

    CameraID active_camera_;
    WindowBase* window_;

    Texture null_texture_;
    Background background_;
    UI ui_interface_;

    ShaderID null_shader_;

    std::vector<Pass> passes_;
    
    mutable boost::mutex scene_lock_;

    sigc::signal<void, Pass&> signal_render_pass_started_;
    sigc::signal<void, Pass&> signal_render_pass_finished_;
};

}

#endif // SCENE_H_INCLUDED
