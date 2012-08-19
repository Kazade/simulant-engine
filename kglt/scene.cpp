#include "glee/GLee.h"
#include "scene.h"
#include "renderer.h"
#include "ui.h"

namespace kglt {

Scene::Scene(WindowBase* window):
    Object(nullptr),
    background_(this),
    active_camera_(DefaultCameraID),
    ui_interface_(new UI(this)),
    window_(window) {

    TemplatedManager<Scene, Mesh, MeshID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Mesh, MeshID>));
    TemplatedManager<Scene, Sprite, SpriteID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Sprite, SpriteID>));
    TemplatedManager<Scene, Camera, CameraID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Camera, CameraID>));
    TemplatedManager<Scene, Text, TextID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Text, TextID>));
    TemplatedManager<Scene, ShaderProgram, ShaderID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_shader_callback));

    background().set_parent(this);

    active_camera_ = new_camera(); //Create a default camera

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
     add_pass(GenericRenderer::create(render_options), VIEWPORT_TYPE_FULL);
}

MeshID Scene::new_mesh(Object *parent) {
    MeshID result = TemplatedManager<Scene, Mesh, MeshID>::manager_new();
    if(parent) {
        mesh(result).set_parent(parent);
    }
    return result;
}

bool Scene::has_mesh(MeshID m) const {
    return TemplatedManager<Scene, Mesh, MeshID>::manager_contains(m);
}

Mesh& Scene::mesh(MeshID m) {
    return TemplatedManager<Scene, Mesh, MeshID>::manager_get(m);
}

void Scene::delete_mesh(MeshID mid) {
    Mesh& obj = mesh(mid);
    obj.destroy_children();
    return TemplatedManager<Scene, Mesh, MeshID>::manager_delete(mid);
}

SpriteID Scene::new_sprite() {
    return TemplatedManager<Scene, Sprite, SpriteID>::manager_new();
}

Sprite& Scene::sprite(SpriteID s) {
    return TemplatedManager<Scene, Sprite, SpriteID>::manager_get(s);
}

bool Scene::has_sprite(SpriteID s) const {
    return TemplatedManager<Scene, Sprite, SpriteID>::manager_contains(s);
}

void Scene::delete_sprite(SpriteID sid) {
    Sprite& obj = sprite(sid);

    obj.destroy_children();

    return TemplatedManager<Scene, Sprite, SpriteID>::manager_delete(sid);
}

MaterialID Scene::new_material() {
    return TemplatedManager<Scene, Material, MaterialID>::manager_new();
}

Material& Scene::material(MaterialID mid) {
    return TemplatedManager<Scene, Material, MaterialID>::manager_get(mid);
}

TextureID Scene::new_texture() {
    static TextureID counter = 0;
    TextureID id = 0;
    {
        boost::mutex::scoped_lock lock(scene_lock_);
        id = ++counter;
    }
    textures_.insert(std::make_pair(id, Texture()));
    return id;
}

void Scene::delete_texture(TextureID tid) {
    {
        boost::mutex::scoped_lock lock(scene_lock_);
        textures_.erase(tid);
    }
}

Texture& Scene::texture(TextureID t) {
    if(t == NullTextureID) {
        return null_texture_;
    }
    
    boost::mutex::scoped_lock lock(scene_lock_);
    
	if(!container::contains(textures_, t)) {
		throw DoesNotExist<TextureID>();
	}
	
    return textures_[t];
}

CameraID Scene::new_camera() {
    return TemplatedManager<Scene, Camera, CameraID>::manager_new();
}

Camera& Scene::camera(CameraID c) {
    return TemplatedManager<Scene, Camera, CameraID>::manager_get(c);
}

void Scene::delete_camera(CameraID cid) {
    Camera& obj = camera(cid);
    obj.destroy_children();
    TemplatedManager<Scene, Camera, CameraID>::manager_delete(cid);
}

ShaderProgram& Scene::shader(ShaderID s) {
    return TemplatedManager<Scene, ShaderProgram, ShaderID>::manager_get(s);
}

ShaderID Scene::new_shader() {
    return TemplatedManager<Scene, ShaderProgram, ShaderID>::manager_new();
}

void Scene::delete_shader(ShaderID s) {
    TemplatedManager<Scene, ShaderProgram, ShaderID>::manager_delete(s);
}
    
FontID Scene::new_font() {
    return TemplatedManager<Scene, Font, FontID>::manager_new();
}

Font& Scene::font(FontID f) {
    return TemplatedManager<Scene, Font, FontID>::manager_get(f);
}

void Scene::delete_font(FontID f) {    
    TemplatedManager<Scene, Font, FontID>::manager_delete(f);
}

TextID Scene::new_text() {
    return TemplatedManager<Scene, Text, TextID>::manager_new();
}

Text& Scene::text(TextID t) {
    return TemplatedManager<Scene, Text, TextID>::manager_get(t);
}

const Text& Scene::text(TextID t) const {
    return TemplatedManager<Scene, Text, TextID>::manager_get(t);
}

void Scene::delete_text(TextID tid) {
    Text& obj = text(tid);
    obj.destroy_children();
    TemplatedManager<Scene, Text, TextID>::manager_delete(tid);
}

OverlayID Scene::new_overlay() {
    return TemplatedManager<Scene, Overlay, OverlayID>::manager_new();
}

bool Scene::has_overlay(OverlayID o) const {
    return TemplatedManager<Scene, Overlay, OverlayID>::manager_contains(o);
}

Overlay& Scene::overlay(OverlayID overlay) {
    return TemplatedManager<Scene, Overlay, OverlayID>::manager_get(overlay);
}

void Scene::delete_overlay(OverlayID oid) {
    Overlay& obj = overlay(oid);
    obj.destroy_children();
    TemplatedManager<Scene, Overlay, OverlayID>::manager_delete(oid);
}

void Scene::init() {
    assert(glGetError() == GL_NO_ERROR);

    
    //Create the null texture
    null_texture_.resize(1, 1);
    null_texture_.set_bpp(32);
    
    null_texture_.data()[0] = 255;
    null_texture_.data()[1] = 255;
    null_texture_.data()[2] = 255;
    null_texture_.data()[3] = 255;
    null_texture_.upload();   
}

std::pair<ShaderID, bool> Scene::find_shader(const std::string& name) {
    std::map<std::string, ShaderID>::const_iterator it = shader_lookup_.find(name);
    if(it == shader_lookup_.end()) {
        return std::make_pair(NullShaderID, false);
    }

    return std::make_pair((*it).second, true);
}

void Scene::update(double dt) {
    for(uint32_t i = 0; i < child_count(); ++i) {
        Object& c = child(i);
        c.update(dt);
	}
}

void Scene::render() {
	/**
	 * Go through all the render passes
	 * set the render options and send the viewport to OpenGL
	 * before going through the scene, objects in the scene
	 * should be able to mark as only being renderered in certain
	 * passes
	 */
	for(Pass& pass: passes_) {
		pass.renderer().set_options(render_options);
		pass.viewport().update_opengl();
		
        signal_render_pass_started_(pass);        
        pass.renderer().render(*this);
        signal_render_pass_finished_(pass);
	}	
}

MeshID Scene::_mesh_id_from_mesh_ptr(Mesh* mesh) {
    return TemplatedManager<Scene, Mesh, MeshID>::_get_object_id_from_ptr(mesh);
}

}
