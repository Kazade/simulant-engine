#ifndef __ANDROID__
	#include <GL/glew.h>
    #include <SDL2/SDL_rwops.h>
#else
    #include <GLES2/gl2.h>
    #include <SDL_rwops.h>
#endif

#include <Rocket/Core.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Core/FontDatabase.h>
#include <Rocket/Core/FileInterface.h>
#include <Rocket/Core/Vertex.h>
#include <Rocket/Core/Types.h>
#include <Rocket/Core/String.h>

#include <kazmath/mat4.h>
#include <thread>

#include "../loader.h"
#include <kazbase/os/path.h>
#include "../window_base.h"
#include "../camera.h"
#include "../render_sequence.h"
#include "../utils/gl_error.h"
#include "../utils/vao_abstraction.h"

#include "interface.h"
#include "ui_private.h"

namespace kglt {
namespace ui {



class RocketFileInterface : public Rocket::Core::FileInterface {
public:
    // Opens a file.
    Rocket::Core::FileHandle Open(const Rocket::Core::String& path) {
        SDL_RWops* ops = SDL_RWFromFile(path.CString(), "r");
        return (Rocket::Core::FileHandle) ops; //Dirty cast
    }

    // Closes a previously opened file.
    void Close(Rocket::Core::FileHandle file) {
        SDL_RWops* ops = (SDL_RWops*) file; //Dirty cast
        SDL_RWclose(ops);
    }

    // Reads data from a previously opened file.
    size_t Read(void* buffer, size_t size, Rocket::Core::FileHandle file) {
        SDL_RWops* ops = (SDL_RWops*) file; //Dirty cast
        return SDL_RWread(ops, buffer, size, 1);
    }

    // Seeks to a point in a previously opened file.
    bool Seek(Rocket::Core::FileHandle file, long offset, int origin) {
        SDL_RWops* ops = (SDL_RWops*) file; //Dirty cast
        SDL_RWseek(ops, offset, origin);
        return true;
    }

    // Returns the current position of the file pointer.
    size_t Tell(Rocket::Core::FileHandle file)  {
        SDL_RWops* ops = (SDL_RWops*) file; //Dirty cast
        return SDL_RWtell(ops);
    }

};

class RocketSystemInterface : public Rocket::Core::SystemInterface {
public:
    RocketSystemInterface(WindowBase& window):
        window_(window) {

    }

    virtual float GetElapsedTime() {
        return window_.total_time();
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
    WindowBase& window_;
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
    RocketRenderInterface(WindowBase& window):
        tmp_vao_(MODIFY_REPEATEDLY_USED_FOR_RENDERING, MODIFY_REPEATEDLY_USED_FOR_RENDERING),
        window_(window) {

        unicode vert_shader = window_.resource_locator().read_file("kglt/materials/ui.vert")->str();
        unicode frag_shader = window_.resource_locator().read_file("kglt/materials/ui.frag")->str();

        L_INFO("UI shaders loaded, creating GPU program");

        shader_ = GPUProgram::create();
        shader_->set_shader_source(SHADER_TYPE_VERTEX, vert_shader);
        shader_->set_shader_source(SHADER_TYPE_FRAGMENT, frag_shader);

        window.idle().run_sync(std::bind(&GPUProgram::build, shader_.get()));
    }

    ~RocketRenderInterface() {
        shader_.reset();
        textures_.clear();
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
        GLStateStash s6(GL_ARRAY_BUFFER_BINDING);

        auto new_group = std::make_shared<CompiledGroup>();
        new_group->vao = std::make_unique<VertexArrayObject>();

        new_group->texture = texture;

        // We have to convert to shorts for GLES compatibility
        std::vector<uint16_t> short_indices(num_indices);
        std::copy(indices, indices + num_indices, &short_indices[0]);

        new_group->vao->vertex_buffer_update(sizeof(Rocket::Core::Vertex) * num_vertices, vertices);
        new_group->vao->index_buffer_update(sizeof(uint16_t) * num_indices, &short_indices[0]);

        new_group->num_indices = num_indices;
        new_group->num_vertices = num_vertices;

        new_group->vao->bind();

        int pos_attrib = shader_->attributes().locate("position");
        int colour_attrib = shader_->attributes().locate("colour");
        int texcoord_attrib = shader_->attributes().locate("tex_coord");

        GLCheck(glEnableVertexAttribArray, pos_attrib);
        GLCheck(vaoVertexAttribPointer,
            pos_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, position))
        );

        GLCheck(glEnableVertexAttribArray, colour_attrib);
        GLCheck(vaoVertexAttribPointer,
            colour_attrib,
            4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, colour))
        );

        GLCheck(glEnableVertexAttribArray, texcoord_attrib);
        GLCheck(vaoVertexAttribPointer,
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
        GLStateStash s6(GL_ARRAY_BUFFER_BINDING);

        // We have to convert to shorts for GLES compatibility
        std::vector<uint16_t> short_indices(num_indices);
        std::copy(indices, indices + num_indices, &short_indices[0]);

        //Update the buffers
        tmp_vao_.vertex_buffer_update(num_vertices * sizeof(Rocket::Core::Vertex), vertices);
        tmp_vao_.index_buffer_update(num_indices * sizeof(uint16_t), &short_indices[0]);
        tmp_vao_.bind();
        tmp_vao_.vertex_buffer_bind();
        tmp_vao_.index_buffer_bind();

        GLCheck(glDisable, GL_DEPTH_TEST);
        GLCheck(glEnable, GL_BLEND);
        GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        prepare_shader(translation);

        int pos_attrib = shader_->attributes().locate("position");
        int colour_attrib = shader_->attributes().locate("colour");
        int texcoord_attrib = shader_->attributes().locate("tex_coord");

        GLCheck(glEnableVertexAttribArray, pos_attrib);
        GLCheck(vaoVertexAttribPointer,
            pos_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, position))
        );

        GLCheck(glEnableVertexAttribArray, colour_attrib);
        GLCheck(vaoVertexAttribPointer,
            colour_attrib,
            4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, colour))
        );

        GLCheck(glEnableVertexAttribArray, texcoord_attrib);
        GLCheck(vaoVertexAttribPointer,
            texcoord_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            BUFFER_OFFSET((int)offsetof(Rocket::Core::Vertex, tex_coord))
        );

        GLCheck(glActiveTexture, GL_TEXTURE0);
        if(texture) {
            GLuint tex_id = textures_[texture]->gl_tex();
            GLCheck(glBindTexture, GL_TEXTURE_2D, tex_id);
        } else {
            GLCheck(glBindTexture, GL_TEXTURE_2D, window_.texture(window_.default_texture_id())->gl_tex());
        }

        GLCheck(glDrawElements, GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
    }

    void prepare_shader(const Rocket::Core::Vector2f& translation) {
        Mat4 transform;
        kmMat4Translation(&transform, translation.x, translation.y, 0);
        transform = this->_projection_matrix * transform;

        assert(shader_);

        shader_->build(); //Build if necessary

        shader_->activate();
        shader_->uniforms().set_mat4x4("modelview_projection", transform);
        shader_->uniforms().set_int("texture_unit", 0);
    }

    void RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation) {
        auto it = compiled_geoms_.find(geometry);

        if(it == compiled_geoms_.end()) {
            throw ValueError("Tried to render invalid UI geometry");
        }

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
            auto tex = textures_.find(geom->texture);
            if(tex == textures_.end()) {
                throw ValueError("Tried to bind invalid UI texture");
            }

            GLuint tex_id = (*tex).second->gl_tex();
            GLCheck(glBindTexture, GL_TEXTURE_2D, tex_id);
        } else {
            GLCheck(glBindTexture, GL_TEXTURE_2D, window_.texture(window_.default_texture_id())->gl_tex());
        }

        prepare_shader(translation);

        GLCheck(glDrawElements, GL_TRIANGLES, geom->num_indices, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
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
        GLCheck(glScissor, x, window_.height() - (y + height), width, height);
    }


    ResourceManager& manager() {
        return window_;
    }


    Mat4 _projection_matrix;
private:
    WindowBase& window_;

    GPUProgram::ptr shader_;

    std::unordered_map<Rocket::Core::TextureHandle, TexturePtr> textures_;
};


static RocketSystemInterface* rocket_system_interface_;
static RocketRenderInterface* rocket_render_interface_;
static RocketFileInterface* rocket_file_interface_;

Interface::Interface(WindowBase &window):
    window_(window),
    impl_(new RocketImpl()) {

}

static int32_t interface_count = 0;

std::vector<unicode> Interface::find_fonts() {
    /*
     * Unfortunately, because Android doesn't easily let you list folders in a portable way
     * we have to hard code the list of fonts. Bummer.
     */

    const std::vector<unicode> FONT_PATHS = {
        "kglt/fonts/opensans/OpenSans-BoldItalic.ttf",
        "kglt/fonts/opensans/OpenSans-Bold.ttf",
        "kglt/fonts/opensans/OpenSans-ExtraBoldItalic.ttf",
        "kglt/fonts/opensans/OpenSans-ExtraBold.ttf",
        "kglt/fonts/opensans/OpenSans-Italic.ttf",
        "kglt/fonts/opensans/OpenSans-LightItalic.ttf",
        "kglt/fonts/opensans/OpenSans-Light.ttf",
        "kglt/fonts/opensans/OpenSans-Regular.ttf",
        "kglt/fonts/opensans/OpenSans-SemiboldItalic.ttf",
        "kglt/fonts/opensans/OpenSans-Semibold.ttf",
        "kglt/fonts/ubuntu/Ubuntu-BI.ttf",
        "kglt/fonts/ubuntu/Ubuntu-B.ttf",
        "kglt/fonts/ubuntu/Ubuntu-C.ttf",
        "kglt/fonts/ubuntu/Ubuntu-LI.ttf",
        "kglt/fonts/ubuntu/Ubuntu-L.ttf",
        "kglt/fonts/ubuntu/Ubuntu-MI.ttf",
        "kglt/fonts/ubuntu/Ubuntu-M.ttf",
        "kglt/fonts/ubuntu/Ubuntu-RI.ttf",
        "kglt/fonts/ubuntu/Ubuntu-R.ttf",
        "kglt/fonts/ubuntu/UbuntuMono-BI.ttf",
        "kglt/fonts/ubuntu/UbuntuMono-B.ttf",
        "kglt/fonts/ubuntu/UbuntuMono-RI.ttf",
        "kglt/fonts/ubuntu/UbuntuMono-R.ttf",
    };

    std::vector<unicode> results;
    for(unicode font: FONT_PATHS) {
        const unicode path = window_.resource_locator().locate_file(font);
        results.push_back(path);
    }

    return results;
}

bool Interface::init() {
    interface_count++;

    if(!rocket_system_interface_) {
        rocket_system_interface_ = new RocketSystemInterface(window_);
        Rocket::Core::SetSystemInterface(rocket_system_interface_);

        rocket_render_interface_ = new RocketRenderInterface(window_);
        Rocket::Core::SetRenderInterface(rocket_render_interface_);

        rocket_file_interface_ = new RocketFileInterface();
        Rocket::Core::SetFileInterface(rocket_file_interface_);

        Rocket::Core::Initialise();

        Rocket::Core::ElementInstancer* element_instancer = new Rocket::Core::ElementInstancerGeneric<CustomDocument>();
        Rocket::Core::Factory::RegisterElementInstancer("body", element_instancer);
        element_instancer->RemoveReference();

        bool font_found = false;
        for(unicode font: find_fonts()) {
            try {
                if(!Rocket::Core::FontDatabase::LoadFontFace(locate_font(font).encode().c_str())) {
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
        Rocket::Core::Vector2i(window_.width(), window_.height())
    );
    impl_->document_ = dynamic_cast<CustomDocument*>(impl_->context_->CreateDocument());
    assert(impl_->document_);
    impl_->document_->set_impl(impl_.get());

    set_styles("body { font-family: \"Ubuntu\"; }");

    return true;
}

void Interface::load_font(const unicode &ttf_file) {
    if(!Rocket::Core::FontDatabase::LoadFontFace(locate_font(ttf_file).encode().c_str())) {
        throw IOError("Couldn't load the font");
    }
}

unicode Interface::locate_font(const unicode& filename) {
    //FIXME: Should be %WINDIR% not C:\Windows
    //FIXME: Should look recursively in /usr/share/fonts
    std::vector<std::string> paths = {
        "/usr/share/fonts",
        "/usr/local/share/fonts"
        "/Library/Fonts",
        "/System/Library/Fonts",
        "C:\\Windows\\fonts"
    };


    return window_.resource_locator().locate_file(filename).encode();

/*
    for(std::string font_dir: paths) {


        if(os::path::exists(os::path::join(font_dir, filename))) {
            return os::path::join(font_dir, filename);
        }
    }

    throw IOError("Unable to locate font: " + filename);*/
}

void Interface::update(float dt) {
    if(!impl_) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
    impl_->context_->Update();
}

void Interface::render(const Mat4 &projection_matrix) {
    set_projection_matrix(projection_matrix); //Set the projection matrix
    RocketRenderInterface* iface = dynamic_cast<RocketRenderInterface*>(impl_->context_->GetRenderInterface());

    iface->_projection_matrix = projection_matrix;
    {
        std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
        impl_->context_->Render();
    }

    set_projection_matrix(Mat4()); //Reset to identity
}

void Interface::set_dimensions(uint16_t width, uint16_t height) {
    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
    impl_->context_->SetDimensions(Rocket::Core::Vector2i(width, height));
}

uint16_t Interface::width() const {
    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
    return impl_->context_->GetDimensions().x;
}

uint16_t Interface::height() const {
    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
    return impl_->context_->GetDimensions().x;
}

ElementList Interface::append(const unicode &tag) {
    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);

    unicode tag_name = tag.strip();

    Rocket::Core::Element* elem = impl_->document_->CreateElement(tag_name.encode().c_str());
    impl_->document_->AppendChild(elem);

    Element result = Element(impl_->document_->get_impl_for_element(elem));

    impl_->document_->Show();

    return ElementList({result});
}

ElementList Interface::_(const unicode &selector) {
    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);

    std::vector<Element> result;
    Rocket::Core::ElementList elements;
    if(selector.starts_with(".")) {
        impl_->document_->GetElementsByClassName(elements, selector.lstrip(".").encode().c_str());
    } else if(selector.starts_with("#")) {
        Rocket::Core::Element* elem = impl_->document_->GetElementById(selector.lstrip("#").encode().c_str());
        if(elem) {
            result.push_back(Element(impl_->document_->get_impl_for_element(elem)));
        }
    } else {
        impl_->document_->GetElementsByTagName(elements, _u("<{0}>").format(selector).encode().c_str());
    }

    for(Rocket::Core::Element* elem: elements) {
        result.push_back(Element(impl_->document_->get_impl_for_element(elem)));
    }

    return ElementList(result);
}

void Interface::set_styles(const std::string& stylesheet_content) {
    std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
    impl_->document_->SetStyleSheet(Rocket::Core::Factory::InstanceStyleSheetString(stylesheet_content.c_str()));
}

Interface::~Interface() {
    try {
        if(impl_ && impl_->context_) {
            std::lock_guard<std::recursive_mutex> lck(impl_->mutex_);
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
