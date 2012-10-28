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
#include "font.h"
#include "text.h"
#include "material.h"
#include "light.h"
#include "scene_group.h"

#include "pipeline.h"

#include "generic/visitor.h"
#include "generic/manager.h"

namespace kglt {

class WindowBase;
class UI;

class Scene :
    public Object,
    public Loadable,
    public generic::TemplatedManager<Scene, Mesh, MeshID>,
    public generic::TemplatedManager<Scene, Camera, CameraID>,
    public generic::TemplatedManager<Scene, Text, TextID>,
    public generic::TemplatedManager<Scene, ShaderProgram, ShaderID>,
    public generic::TemplatedManager<Scene, Font, FontID>,
    public generic::TemplatedManager<Scene, Material, MaterialID>,
    public generic::TemplatedManager<Scene, Texture, TextureID>,
    public generic::TemplatedManager<Scene, Light, LightID>,
    public generic::TemplatedManager<Scene, SceneGroup, SceneGroupID> {

public:
    VIS_DEFINE_VISITABLE();

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the scene");
    }

    Scene(WindowBase* window);
    ~Scene();

    SceneGroupID new_scene_group();
    SceneGroup& scene_group(SceneGroupID group=0);
    void delete_scene_group(SceneGroupID group);

    MeshID new_mesh(Object* parent=nullptr);
    Mesh& mesh(MeshID m);
    void delete_mesh(MeshID mid);

    CameraID new_camera();
    TextureID new_texture();
    ShaderID new_shader();
    FontID new_font();
    TextID new_text();
    MaterialID new_material(MaterialID clone_from=0);
    LightID new_light(Object* parent=nullptr, LightType type=LIGHT_TYPE_POINT);

    bool has_mesh(MeshID m) const;

    Camera& camera(CameraID c = DefaultCameraID);
    Texture& texture(TextureID t);
    ShaderProgram& shader(ShaderID s);
    Font& font(FontID f);
    Material& material(MaterialID material);
    Light& light(LightID light);

    Text& text(TextID t);
    const Text& text(TextID t) const;

    std::pair<ShaderID, bool> find_shader(const std::string& name);


    void delete_texture(TextureID tid);
    void delete_camera(CameraID cid);
    void delete_text(TextID tid);
    void delete_shader(ShaderID s);
    void delete_font(FontID f);
    void delete_material(MaterialID m);
    void delete_light(LightID light_id);

    void init();
    void render();
    void update(double dt);

    WindowBase& window() { return *window_; }

    MeshID _mesh_id_from_mesh_ptr(Mesh* mesh);

    UI& ui() { return *ui_interface_; }

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(this);
        obj._initialize(*this);
    }

    void post_create_shader_callback(ShaderProgram& obj, ShaderID id) {
        shader_lookup_[obj.name()] = id;
    }

    MaterialID default_material() const { return default_material_; }
    ShaderID default_shader() const { return default_shader_; }

    SceneGroupID default_scene_group() const { return default_scene_group_; }

    kglt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const kglt::Colour& c) { ambient_light_ = c; }

    sigc::signal<void, Mesh&>& signal_mesh_created() { return signal_mesh_created_; }
    sigc::signal<void, Mesh&>& signal_mesh_destroyed() { return signal_mesh_destroyed_; }

    sigc::signal<void, Light&>& signal_light_created() { return signal_light_created_; }
    sigc::signal<void, Light&>& signal_light_destroyed() { return signal_light_destroyed_; }

    Pipeline& pipeline() { return *pipeline_; }
private:
    std::map<std::string, ShaderID> shader_lookup_;

    WindowBase* window_;

    CameraID default_camera_;
    SceneGroupID default_scene_group_;
    TextureID default_texture_;
    ShaderID default_shader_;
    ShaderID phong_shader_;
    MaterialID default_material_;
    kglt::Colour ambient_light_;

    void initialize_defaults();

    std::tr1::shared_ptr<UI> ui_interface_;

    Pipeline::ptr pipeline_;

    sigc::signal<void, Mesh&> signal_mesh_created_;
    sigc::signal<void, Mesh&> signal_mesh_destroyed_;

    sigc::signal<void, Light&> signal_light_created_;
    sigc::signal<void, Light&> signal_light_destroyed_;
};

}

#endif // SCENE_H_INCLUDED
