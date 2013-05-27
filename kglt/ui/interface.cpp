#include <GL/GLee.h>
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
#include "../pipeline.h"

#include "interface.h"
#include "ui_private.h"

namespace kglt {
namespace ui {

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

class RocketRenderInterface : public Rocket::Core::RenderInterface {
public:
    RocketRenderInterface(WindowBase& window):
        window_(window) {

        //At the start of each frame, clear out any rendered geometry
        window_.signal_frame_started().connect(sigc::mem_fun(this, &RocketRenderInterface::clear_objects));

    }

    bool LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source) {
        kglt::TexturePtr tex = manager().texture(manager().new_texture()).lock();
        window_.loader_for(source.CString())->into(*tex);

        //FIXME: Passing around a reference isn't entirely safe...
        //but at least this makes sure that the upload happens in the main thread
        window_.idle().add_once([&]() {
            tex->flip_vertically();
            tex->upload(true, false, false, true);
        });

        texture_handle = tex->id().value();

        textures_[texture_handle] = tex; //Hold for ref-counting
        return true;
    }

    bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& dimensions) {
        kglt::TexturePtr tex = manager().texture(manager().new_texture()).lock();

        uint32_t data_size = (dimensions.x * dimensions.y * 4);
        tex->resize(dimensions.x, dimensions.y);
        tex->set_bpp(32);

        tex->data().assign(source, source + data_size);
        tex->upload(true, false, false, true);

        texture_handle = tex->id().value();

        textures_[texture_handle] = tex; //Hold for ref-counting
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

        kmMat4 projection;
        kmMat4OrthographicProjection(&projection, 0, window_.width(), window_.height(), 0, -1, 1);

        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection.mat);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(translation.x, translation.y, 0.0);

        glUseProgram(0);
        glActiveTexture(GL_TEXTURE0);

        if(texture) {
            glEnable(GL_TEXTURE_2D);
            GLuint tex_id = textures_[texture]->gl_tex();
            glBindTexture(GL_TEXTURE_2D, tex_id);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        } else {
            glDisable(GL_TEXTURE_2D);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_ALPHA_TEST);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glBegin(GL_TRIANGLES);
        for(int32_t i = 0; i < num_indices; ++i) {
            Rocket::Core::Vertex* v = vertices + indices[i];
            glColor4ub(
                v->colour.red,
                v->colour.green,
                v->colour.blue,
                v->colour.alpha
            );
            glTexCoord2f(v->tex_coord.x, v->tex_coord.y);
            glVertex2f(v->position.x, v->position.y);
        }
        glEnd();

        glPopMatrix();
    }


    void EnableScissorRegion(bool enable) {
        if(enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    void SetScissorRegion(int x, int y, int width, int height) {
        glScissor(x, window_.height() - (y + height), width, height);
    }

    void clear_objects() {

    }

    ResourceManager& manager() {
        return window_.scene();
    }

private:
    WindowBase& window_;

    std::map<Rocket::Core::TextureHandle, TexturePtr> textures_;
};


static std::shared_ptr<RocketSystemInterface> rocket_system_interface_;
static std::shared_ptr<RocketRenderInterface> rocket_render_interface_;

Interface::Interface(WindowBase& window, uint32_t width_in_pixels, uint32_t height_in_pixels):
    window_(window),
    width_(width_in_pixels),
    height_(height_in_pixels),
    impl_(new RocketImpl()) {

    window.signal_pre_swap().connect(sigc::mem_fun(this, &Interface::render));
}

bool Interface::init() {
    if(!rocket_system_interface_) {
        rocket_system_interface_.reset(new RocketSystemInterface(window_));
        Rocket::Core::SetSystemInterface(rocket_system_interface_.get());

        rocket_render_interface_.reset(new RocketRenderInterface(window_));
        Rocket::Core::SetRenderInterface(rocket_render_interface_.get());

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

    //FIXME: Change name for each interface
    impl_->context_ = Rocket::Core::CreateContext("default", Rocket::Core::Vector2i(width_, height_));
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
    impl_->context_->Update();
}

void Interface::render() {
    impl_->context_->Render();
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

    Rocket::Core::Shutdown();
}

}
}
