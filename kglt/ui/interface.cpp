#ifndef __ANDROID__
	#include <GL/glew.h>
#else
	#include <GLES3/gl3.h>
#endif

#include <Rocket/Core.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Core/FontDatabase.h>
#include <Rocket/Core/Vertex.h>
#include <Rocket/Core/Types.h>
#include <Rocket/Core/String.h>

#include <kazmath/mat4.h>

#include "../loader.h"
#include "../kazbase/string.h"
#include "../kazbase/os/path.h"
#include "../window_base.h"
#include "../scene.h"
#include "../camera.h"
#include "../render_sequence.h"
#include "../utils/gl_error.h"

#include "interface.h"
#include "ui_private.h"

namespace kglt {
namespace ui {

class RocketSystemInterface : public Rocket::Core::SystemInterface {
public:
    RocketSystemInterface(Scene& scene):
        scene_(scene) {

    }

    virtual float GetElapsedTime() {
        return scene_.window().total_time();
    }

    //The default interface is weird and strips the leading slash from absolute paths
    //this works around that. I have no idea why it was designed to do that.
    virtual void JoinPath(Rocket::Core::String& translated_path, const Rocket::Core::String& document_path, const Rocket::Core::String& path) {
        if (path.Substring(0, 1) == "/") {
            translated_path = path;
            return;
        }

        Rocket::Core::SystemInterface::JoinPath(translated_path, document_path, path);
    }

private:
    Scene& scene_;
};

struct CompiledGroup {
    std::unique_ptr<VertexArrayObject> vao;
    Rocket::Core::TextureHandle texture;

    int num_indices;
    int num_vertices;
};

class RocketRenderInterface : public Rocket::Core::RenderInterface {
private:
    std::unordered_map<Rocket::Core::CompiledGeometryHandle, std::shared_ptr<CompiledGroup>> compiled_geoms_;

    VertexArrayObject tmp_vao_;

public:
    RocketRenderInterface(Scene& scene):
        scene_(scene),
        tmp_vao_(MODIFY_REPEATEDLY_USED_FOR_RENDERING, MODIFY_REPEATEDLY_USED_FOR_RENDERING) {

        unicode vert_shader = scene.window().resource_locator().read_file("kglt/materials/ui.vert")->str();
        unicode frag_shader = scene.window().resource_locator().read_file("kglt/materials/ui.frag")->str();

        shader_ = scene_.new_shader_from_files(vert_shader, frag_shader);
        shader_reference_ = scene_.shader(shader_).lock(); //Prevent GC
    }

    bool LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source) {
        auto tex = manager().texture(manager().new_texture_from_file(source.CString()));

        tex->flip_vertically();
        tex->upload(true, false, false, true);

        texture_handle = tex->id().value();

        textures_[texture_handle] = tex.__object; //Hold for ref-counting
        return true;
    }

    bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& dimensions) {
        auto tex = manager().texture(manager().new_texture());

        uint32_t data_size = (dimensions.x * dimensions.y * 4);
        tex->resize(dimensions.x, dimensions.y);
        tex->set_bpp(32);

        tex->data().assign(source, source + data_size);
        tex->upload(true, false, false, true);

        texture_handle = tex->id().value();

        textures_[texture_handle] = tex.__object; //Hold for ref-counting
        return true;
    }

    void ReleaseTexture(Rocket::Core::TextureHandle texture) {
        textures_.erase(texture); //Decrement the ref-count
    }

    Rocket::Core::CompiledGeometryHandle CompileGeometry(
            Rocket::Core::Vertex* vertices,
            int num_vertices,
            int* indices,
            int num_indices,
            Rocket::Core::TextureHandle texture) {

        GLStateStash s1(GL_VERTEX_ARRAY_BINDING);

        auto new_group = std::make_shared<CompiledGroup>();
        new_group->vao = std::make_unique<VertexArrayObject>();

        new_group->texture = texture;

        new_group->vao->vertex_buffer_update(sizeof(Rocket::Core::Vertex) * num_vertices, vertices);
        new_group->vao->index_buffer_update(sizeof(int) * num_indices, indices);

        new_group->num_indices = num_indices;
        new_group->num_vertices = num_vertices;

        new_group->vao->bind();
        new_group->vao->vertex_buffer_bind();
        new_group->vao->index_buffer_bind();

        int pos_attrib = -1, colour_attrib = -1, texcoord_attrib = -1;
        {
            auto shader = scene_.shader(shader_).lock();
            pos_attrib = shader->get_attrib_loc("position");
            colour_attrib = shader->get_attrib_loc("colour");
            texcoord_attrib = shader->get_attrib_loc("tex_coord");
        }

        GLCheck(glEnableVertexAttribArray, pos_attrib);
        GLCheck(glVertexAttribPointer,
            pos_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, position))
        );

        GLCheck(glEnableVertexAttribArray, colour_attrib);
        GLCheck(glVertexAttribPointer,
            colour_attrib,
            4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, colour))
        );

        GLCheck(glEnableVertexAttribArray, texcoord_attrib);
        GLCheck(glVertexAttribPointer,
            texcoord_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, tex_coord))
        );

        /*FIXME: WARNING: This is a cast from pointer to an unsigned long, might not be portable! */
        Rocket::Core::CompiledGeometryHandle res = (unsigned long) new_group.get();

        compiled_geoms_[res] = new_group;

        return res;
    }

    void RenderGeometry(
            Rocket::Core::Vertex* vertices,
            int num_vertices, int* indices,
            int num_indices, Rocket::Core::TextureHandle texture,
            const Rocket::Core::Vector2f& translation) {

        GLStateStash s1(GL_VERTEX_ARRAY_BINDING);
        GLStateStash s2(GL_DEPTH_TEST);
        GLStateStash s3(GL_BLEND);
        GLStateStash s4(GL_CURRENT_PROGRAM);
        GLStateStash s5(GL_TEXTURE_BINDING_2D);

        //Update the buffers
        tmp_vao_.vertex_buffer_update(num_vertices * sizeof(Rocket::Core::Vertex), vertices);
        tmp_vao_.index_buffer_update(num_indices * sizeof(int), indices);
        tmp_vao_.bind();
        tmp_vao_.vertex_buffer_bind();
        tmp_vao_.index_buffer_bind();

        GLCheck(glDisable, GL_DEPTH_TEST);
        GLCheck(glEnable, GL_BLEND);
        GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        prepare_shader(translation);

        int pos_attrib = -1, colour_attrib = -1, texcoord_attrib = -1;
        {
            auto shader = scene_.shader(shader_).lock();
            pos_attrib = shader->get_attrib_loc("position");
            colour_attrib = shader->get_attrib_loc("colour");
            texcoord_attrib = shader->get_attrib_loc("tex_coord");
        }

        GLCheck(glEnableVertexAttribArray, pos_attrib);
        GLCheck(glVertexAttribPointer, 
            pos_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, position))
        );

        GLCheck(glEnableVertexAttribArray, colour_attrib);
        GLCheck(glVertexAttribPointer, 
            colour_attrib,
            4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, colour))
        );

        GLCheck(glEnableVertexAttribArray, texcoord_attrib);
        GLCheck(glVertexAttribPointer,
            texcoord_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, tex_coord))
        );

        GLCheck(glActiveTexture, GL_TEXTURE0);
        if(texture) {
            GLuint tex_id = textures_[texture]->gl_tex();
            GLCheck(glBindTexture, GL_TEXTURE_2D, tex_id);
        } else {
            GLCheck(glBindTexture, GL_TEXTURE_2D, scene_.texture(scene_.default_texture_id())->gl_tex());
        }

        GLCheck(glDrawElements, GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
    }

    void prepare_shader(const Rocket::Core::Vector2f& translation) {
        Mat4 transform;
        kmMat4Translation(&transform, translation.x, translation.y, 0);
        transform = this->_projection_matrix * transform;

        auto shader = scene_.shader(shader_).lock();
        shader->activate();
        shader->params().set_mat4x4("modelview_projection", transform);
        shader->params().set_int("texture_unit", 0);
    }

    void RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation) {
        auto it = compiled_geoms_.find(geometry);
        assert(it != compiled_geoms_.end());

        auto geom = (*it).second;

        GLStateStash s1(GL_VERTEX_ARRAY_BINDING);
        GLStateStash s2(GL_DEPTH_TEST);
        GLStateStash s3(GL_BLEND);
        GLStateStash s4(GL_CURRENT_PROGRAM);
        GLStateStash s5(GL_TEXTURE_BINDING_2D);

        GLCheck(glDisable, GL_DEPTH_TEST);
        GLCheck(glEnable, GL_BLEND);
        GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        geom->vao->bind();

        GLCheck(glActiveTexture, GL_TEXTURE0);
        if(geom->texture) {
            GLuint tex_id = textures_[geom->texture]->gl_tex();
            GLCheck(glBindTexture, GL_TEXTURE_2D, tex_id);
        } else {
            GLCheck(glBindTexture, GL_TEXTURE_2D, scene_.texture(scene_.default_texture_id())->gl_tex());
        }

        prepare_shader(translation);

        GLCheck(glDrawElements, GL_TRIANGLES, geom->num_indices, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
    }

    void ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry) {
        auto it = compiled_geoms_.find(geometry);
        if(it == compiled_geoms_.end()) {
            return;
        }

        compiled_geoms_.erase(it);
    }

    void EnableScissorRegion(bool enable) {
        if(enable) {
            GLCheck(glEnable, GL_SCISSOR_TEST);
        } else {
            GLCheck(glDisable, GL_SCISSOR_TEST);
        }
    }

    void SetScissorRegion(int x, int y, int width, int height) {
        GLCheck(glScissor, x, scene_.window().height() - (y + height), width, height);
    }


    ResourceManager& manager() {
        return scene_;
    }


    Mat4 _projection_matrix;
private:
    Scene& scene_;

    ShaderID shader_ = ShaderID();
    ShaderProgram::ptr shader_reference_;

    std::map<Rocket::Core::TextureHandle, TexturePtr> textures_;
};


static RocketSystemInterface* rocket_system_interface_;
static RocketRenderInterface* rocket_render_interface_;

Interface::Interface(Scene &scene):
    scene_(scene),
    impl_(new RocketImpl()) {

}

static int32_t interface_count = 0;

bool Interface::init() {
    interface_count++;

    if(!rocket_system_interface_) {
        rocket_system_interface_ = new RocketSystemInterface(scene_);
        Rocket::Core::SetSystemInterface(rocket_system_interface_);

        rocket_render_interface_ = new RocketRenderInterface(scene_);
        Rocket::Core::SetRenderInterface(rocket_render_interface_);

        Rocket::Core::Initialise();

        bool font_found = false;
        for(std::string font: {
            "kglt/fonts/ubuntu/Ubuntu-R.ttf",
            "kglt/fonts/ubuntu/Ubuntu-B.ttf",
            "kglt/fonts/ubuntu/Ubuntu-I.ttf",
            "kglt/fonts/ubuntu/Ubuntu-BI.ttf"
            "kglt/fonts/ubuntu/UbuntuMono-R.ttf",
            "kglt/fonts/ubuntu/UbuntuMono-B.ttf",
            "kglt/fonts/ubuntu/UbuntuMono-I.ttf",
            "kglt/fonts/ubuntu/UbuntuMono-BI.ttf"
        }) {
            try {
                if(!Rocket::Core::FontDatabase::LoadFontFace(locate_font(font).c_str())) {
                    throw IOError("Couldn't load the font");
                }
                font_found = true;
            } catch(IOError& e) {
                continue;
            }
        }

        if(!font_found) {
            throw IOError("Unable to find a default font");
        }
    }

    //Change name for each interface using this (dirty, but works)
    impl_->context_ = Rocket::Core::CreateContext(
        _u("context_{0}").format(int64_t(this)).encode().c_str(),
        Rocket::Core::Vector2i(scene_.window().width(), scene_.window().height())
    );
    impl_->document_ = impl_->context_->CreateDocument();
    set_styles("body { font-family: \"Ubuntu\"; }");

    return true;
}

std::string Interface::locate_font(const std::string& filename) {
    //FIXME: Should be %WINDIR% not C:\Windows
    //FIXME: Should look recursively in /usr/share/fonts
    std::vector<std::string> paths = {
        "/usr/share/fonts",
        "/usr/local/share/fonts"
        "/Library/Fonts",
        "/System/Library/Fonts",
        "C:\\Windows\\fonts"
    };


    return scene_.window().resource_locator().locate_file(filename).encode();

/*
    for(std::string font_dir: paths) {


        if(os::path::exists(os::path::join(font_dir, filename))) {
            return os::path::join(font_dir, filename);
        }
    }

    throw IOError("Unable to locate font: " + filename);*/
}

void Interface::update(float dt) {
    impl_->context_->Update();
}

void Interface::render(const Mat4 &projection_matrix) {
    set_projection_matrix(projection_matrix); //Set the projection matrix
    RocketRenderInterface* iface = dynamic_cast<RocketRenderInterface*>(impl_->context_->GetRenderInterface());

    iface->_projection_matrix = projection_matrix;
    impl_->context_->Render();

    set_projection_matrix(Mat4()); //Reset to identity
}

void Interface::set_dimensions(uint16_t width, uint16_t height) {
    impl_->context_->SetDimensions(Rocket::Core::Vector2i(width, height));
}

uint16_t Interface::width() const {
    return impl_->context_->GetDimensions().x;
}

uint16_t Interface::height() const {
    return impl_->context_->GetDimensions().x;
}

Element Interface::append(const std::string& tag) {
    std::string tag_name = str::strip(tag);

    Rocket::Core::Element* elem = impl_->document_->CreateElement(tag_name.c_str());
    impl_->document_->AppendChild(elem);

    Element result = Element(
        std::shared_ptr<ElementImpl>(
            new ElementImpl(elem)
        )
    );

    impl_->document_->Show();

    return result;
}

ElementList Interface::_(const std::string& selector) {
    std::vector<Element> result;
    Rocket::Core::ElementList elements;
    if(str::starts_with(selector, ".")) {
        impl_->document_->GetElementsByClassName(elements, str::strip(selector, ".").c_str());
    } else if(str::starts_with(selector, "#")) {
        Rocket::Core::Element* elem = impl_->document_->GetElementById(str::strip(selector, "#").c_str());
        if(elem) {
            result.push_back(
                Element(std::shared_ptr<ElementImpl>(new ElementImpl(elem)))
            );
        }
    } else {
        impl_->document_->GetElementsByTagName(elements, ("<" + selector + ">").c_str());
    }

    for(Rocket::Core::Element* elem: elements) {
        result.push_back(Element(std::shared_ptr<ElementImpl>(new ElementImpl(elem))));
    }

    return ElementList(result);
}

void Interface::set_styles(const std::string& stylesheet_content) {
    impl_->document_->SetStyleSheet(Rocket::Core::Factory::InstanceStyleSheetString(stylesheet_content.c_str()));
}

Interface::~Interface() {
    try {
        if(impl_ && impl_->context_) {
            impl_->context_->RemoveReference();
        }
    } catch(...) {
        L_ERROR("Error removing reference to the context");
    }

    //Shutdown if this is the last interface
    if(interface_count-- == 0) {
        rocket_system_interface_->RemoveReference();
        rocket_render_interface_->RemoveReference();
        Rocket::Core::Shutdown();
    }
}

}
}
