#include "glee/GLee.h"
#include "scene.h"
#include "renderer.h"

namespace kglt {

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

void Scene::delete_mesh(MeshID mid) {
    Mesh& m = mesh(mid);
    m.set_parent(nullptr);
    meshes_.erase(mid);
}

SpriteID Scene::new_sprite() {
    static SpriteID counter = 0;
    SpriteID id = ++counter;
    sprites_.insert(std::make_pair(id, Sprite::ptr(new Sprite)));
    
    Sprite& sprite = *sprites_[id];
    sprite.set_parent(this);
    
    return id;
}

Sprite& Scene::sprite(SpriteID s) {
    //FIXME: assert contains
    return *sprites_[s];
}

TextureID Scene::new_texture() {
    static TextureID counter = 0;
    TextureID id = ++counter;
    Texture& tex = textures_[id];
    return id;
}

Texture& Scene::texture(TextureID t) {
    if(t == NullTextureID) {
        return null_texture_;
    }
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

ShaderProgram& Scene::shader(ShaderID s) {
	auto it = shaders_.find(s);
	assert(it != shaders_.end());
	return *(*it).second;    
}

ShaderID Scene::new_shader() {
    static ShaderID counter = 0;
    ShaderID id = counter++; //The first shader should be 0 - or the default shader    
    shaders_.insert(std::make_pair(id, ShaderProgram::ptr(new ShaderProgram)));
    return id;
}
    
Camera& Scene::camera(CameraID c) {
    //FIXME: Assert

    if(c == 0) {
        return *cameras_[current_camera_];
    }

    return *cameras_[c];
}

void Scene::init() {
    assert(glGetError() == GL_NO_ERROR);
    ShaderProgram& def = shader(new_shader()); //Create a default shader;
            
    assert(glGetError() == GL_NO_ERROR);
        
    def.add_and_compile(SHADER_TYPE_VERTEX, kglt::get_default_vert_shader_120());
    def.add_and_compile(SHADER_TYPE_FRAGMENT, kglt::get_default_frag_shader_120());
    def.activate();
    
    //Bind the vertex attributes for the default shader and relink
    def.bind_attrib(0, "vertex_position");
    def.bind_attrib(1, "vertex_texcoord_1");
    def.bind_attrib(2, "vertex_diffuse");
    def.relink();
    
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
	for(std::pair<const ShaderID, ShaderProgram::ptr>& shader: shaders_) {
		if(shader.second->name() == name) {
			return std::make_pair(shader.first, true);
		}
	}

	return std::make_pair(NullShaderID, false);
}

void Scene::update(double dt) {
	for(Object* child: children_) {
		child->update(dt);
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
		
		pass.renderer().start_render(this);
		this->accept(pass.renderer());
		pass.renderer().finish_render();
	}	
}

MeshID Scene::_mesh_id_from_mesh_ptr(Mesh* mesh) {
	for(std::pair<MeshID, Mesh::ptr> pair: meshes_) {
		if(pair.second.get() == mesh) {
			return pair.first;
		}
	}
	
	return 0;
}

}
