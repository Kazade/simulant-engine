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
#include "pass.h"
#include "background.h"
#include "font.h"
#include "text.h"
#include "overlay.h"
#include "material.h"
#include "light.h"

#include "rendering/generic_renderer.h"
#include "partitioner.h"

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
    public generic::TemplatedManager<Scene, Overlay, OverlayID>,
    public generic::TemplatedManager<Scene, Material, MaterialID>,
    public generic::TemplatedManager<Scene, Texture, TextureID>,
    public generic::TemplatedManager<Scene, Light, LightID> {

public:
    VIS_DEFINE_VISITABLE();

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the scene");
    }

    Scene(WindowBase* window);
    ~Scene();

    MeshID new_mesh(Object* parent=nullptr);
    CameraID new_camera();
    TextureID new_texture();
    ShaderID new_shader();
    FontID new_font();
    TextID new_text();
    OverlayID new_overlay(); ///< Creates a new overlay
    MaterialID new_material();
    LightID new_light(Object* parent=nullptr, LightType type=LIGHT_TYPE_POINT);

    bool has_mesh(MeshID m) const;
    bool has_overlay(OverlayID o) const;

    Mesh& mesh(MeshID m);
    Camera& camera(CameraID c = DefaultCameraID);
    Texture& texture(TextureID t);
    ShaderProgram& shader(ShaderID s);
    Font& font(FontID f);
    Overlay& overlay(OverlayID overlay); ///< Grabs an overlay by its ID
    Material& material(MaterialID material);
    Light& light(LightID light);

    Camera& active_camera() { return camera(active_camera_); }
    void set_active_camera(CameraID cam) { active_camera_ = cam; }

    Text& text(TextID t);
    const Text& text(TextID t) const;

    std::pair<ShaderID, bool> find_shader(const std::string& name);

    void delete_mesh(MeshID mid);
    void delete_texture(TextureID tid);
    void delete_camera(CameraID cid);
    void delete_text(TextID tid);
    void delete_shader(ShaderID s);
    void delete_font(FontID f);
    void delete_overlay(OverlayID overlay); ///< Deletes an overlay by its ID
    void delete_material(MaterialID m);
    void delete_light(LightID light_id);

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

    uint32_t pass_count() const { return passes_.size(); }
    Pass& pass(uint32_t index = 0) { return passes_.at(index); }

    MeshID _mesh_id_from_mesh_ptr(Mesh* mesh);

    Background& background() { return background_; }
    UI& ui() { return *ui_interface_; }

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

    uint32_t overlay_count() const { ///< Returns the number of overlays in the scene
        return TemplatedManager<Scene, Overlay, OverlayID>::objects_.size();
    }

    Overlay& overlay_ordered_by_zindex(uint32_t idx) { ///< Returns an overlay by index into a sorted list by zindex
        std::vector<Overlay::ptr> overlays;
        for(std::pair<OverlayID, Overlay::ptr> pair: TemplatedManager<Scene, Overlay, OverlayID>::objects_) {
            overlays.push_back(pair.second);
        }

        std::sort(overlays.begin(), overlays.end(), [](Overlay::ptr x, Overlay::ptr y) { return x->zindex() < y->zindex(); });

        return *overlays[idx];
    }

    MaterialID default_material() const { return default_material_; }
    ShaderID default_shader() const { return default_shader_; }

    Partitioner& partitioner() { return *partitioner_; }

    kglt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const kglt::Colour& c) { ambient_light_ = c; }

private:
    std::map<std::string, ShaderID> shader_lookup_;

    CameraID active_camera_;
    WindowBase* window_;

    TextureID default_texture_;

    ShaderID default_shader_;
    ShaderID phong_shader_;
    MaterialID default_material_;
    kglt::Colour ambient_light_;

    void initialize_defaults();

    Background background_;
    std::tr1::shared_ptr<UI> ui_interface_;

    std::vector<Pass> passes_;

    sigc::signal<void, Pass&> signal_render_pass_started_;
    sigc::signal<void, Pass&> signal_render_pass_finished_;

    Partitioner::ptr partitioner_;
};

}

#endif // SCENE_H_INCLUDED
