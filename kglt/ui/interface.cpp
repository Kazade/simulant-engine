#ifndef __ANDROID__
	#include <GL/glew.h>
#else
	#include <GLES2/gl2.h>
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

class RocketRenderInterface : public Rocket::Core::RenderInterface {
public:
    RocketRenderInterface(Scene& scene, const Interface& interface):
        scene_(scene),
        interface_(interface) {

        unicode vert_shader = scene.window().resource_locator().read_file("kglt/materials/ui.vert")->str();
        unicode frag_shader = scene.window().resource_locator().read_file("kglt/materials/ui.frag")->str();

        shader_ = scene_.new_shader_from_files(vert_shader, frag_shader);
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

    void RenderGeometry(
        Rocket::Core::Vertex* vertices,
        int num_vertices,
        int* indices,
        int num_indices,
        Rocket::Core::TextureHandle texture,
        const Rocket::Core::Vector2f& translation) {

        /* FIXME: Translate by just adding the vector to the positions? or
         * setting the modelview matrix? */

        Mat4 transform;
        kmMat4Translation(&transform, translation.x, translation.y, 0);

        int pos_attrib = -1, colour_attrib = -1, texcoord_attrib = -1;

        {
            auto shader = scene_.shader(shader_).lock();
            shader->params().set_mat4x4("modelview_projection", transform * interface_.projection_matrix());
            pos_attrib = shader->get_attrib_loc("position");
            colour_attrib = shader->get_attrib_loc("colour");
            texcoord_attrib = shader->get_attrib_loc("texcoord");

            assert(pos_attrib > -1);
            assert(colour_attrib > -1);
            assert(texcoord_attrib > -1);
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnableVertexAttribArray(pos_attrib);
        glEnableVertexAttribArray(colour_attrib);

        glVertexAttribPointer(
            pos_attrib,
            2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
            &vertices[0].position
        );

        glVertexAttribPointer(
            colour_attrib,
            4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rocket::Core::Vertex),
            &vertices[0].colour
        );

        if(texture) {
            glEnable(GL_TEXTURE_2D);
            GLuint tex_id = textures_[texture]->gl_tex();
            glBindTexture(GL_TEXTURE_2D, tex_id);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

            glEnableVertexAttribArray(texcoord_attrib);

            glVertexAttribPointer(
                texcoord_attrib,
                2, GL_FLOAT, GL_FALSE, sizeof(Rocket::Core::Vertex),
                &vertices[0].tex_coord
            );
        } else {
            glDisable(GL_TEXTURE_2D);
            glDisableVertexAttribArray(texcoord_attrib);
        }

        glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }


    void EnableScissorRegion(bool enable) {
        if(enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    void SetScissorRegion(int x, int y, int width, int height) {
        glScissor(x, scene_.window().height() - (y + height), width, height);
    }


    ResourceManager& manager() {
        return scene_;
    }

private:
    Scene& scene_;
    const Interface& interface_;
    ShaderID shader_ = ShaderID();

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

        rocket_render_interface_ = new RocketRenderInterface(scene_, *this);
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
    impl_->context_->RemoveReference();

    //Shutdown if this is the last interface
    if(interface_count-- == 0) {
        rocket_system_interface_->RemoveReference();
        rocket_render_interface_->RemoveReference();
        Rocket::Core::Shutdown();
    }
}

}
}
