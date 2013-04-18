#include <GL/GLee.h>
#include <Rocket/Core.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Core/FontDatabase.h>
#include <Rocket/Core/Vertex.h>
#include <Rocket/Core/Types.h>
#include <Rocket/Core/String.h>

#include <kazmath/mat4.h>

#include "../kazbase/os/path.h"
#include "../window_base.h"
#include "../scene.h"
#include "../camera.h"
#include "../pipeline.h"

#include "interface.h"

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
        kglt::Texture& tex = manager().texture(manager().new_texture());
        window_.loader_for(source.CString())->into(tex);

        window_.idle().add_once(std::tr1::bind(&kglt::Texture::upload, tex, true, false, false, true));

        textures_[texture_handle] = tex.id();

        return true;
    }

    bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& dimensions) {
        kglt::Texture& tex = manager().texture(manager().new_texture());

        uint32_t data_size = (dimensions.x * dimensions.y * 4);
        tex.resize(dimensions.x, dimensions.y);
        tex.set_bpp(32);

        tex.data().assign(source, source + data_size);
        tex.upload(true, false, false, true);

        textures_[texture_handle] = tex.id();

        return true;
    }

    void ReleaseTexture(Rocket::Core::TextureHandle texture) {
        kglt::TextureID id = textures_[texture];
        manager().delete_texture(id);
        textures_.erase(texture);
    }

    void RenderGeometry(
        Rocket::Core::Vertex* vertices,
        int num_vertices,
        int* indices,
        int num_indices,
        Rocket::Core::TextureHandle texture,
        const Rocket::Core::Vector2f& translation) {

        logging::warn("Not implemented", __FILE__, __LINE__);


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
            GLuint tex_id = manager().texture(textures_[texture]).gl_tex();
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
        logging::warn("Not implemented", __FILE__, __LINE__);
        if(enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    void SetScissorRegion(int x, int y, int width, int height) {
        logging::warn("Not implemented", __FILE__, __LINE__);
        glScissor(x, window_.height() - (y + height), width, height);
    }

    //Our magic
    void register_context(Rocket::Core::Context* context, kglt::ResourceManager* subscene) {
        if(contexts_.find(context) != contexts_.end()) {
            throw std::logic_error("Tried to register the same context twice");
        }
        contexts_[context] = subscene;
    }

    void unregister_context(Rocket::Core::Context* context) {
        contexts_.erase(context);
    }

    void clear_objects() {

    }

    ResourceManager& manager() {
        Rocket::Core::Context* context = GetContext();
        return *contexts_[context];
    }

private:
    WindowBase& window_;

    std::map<Rocket::Core::Context*, kglt::ResourceManager*> contexts_;
    std::map<Rocket::Core::TextureHandle, kglt::TextureID> textures_;
};


static std::tr1::shared_ptr<RocketSystemInterface> rocket_system_interface_;
static std::tr1::shared_ptr<RocketRenderInterface> rocket_render_interface_;

Interface::Interface(WindowBase& window, uint32_t width_in_pixels, uint32_t height_in_pixels):
    window_(window),
    width_(width_in_pixels),
    height_(height_in_pixels) {

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
        for(std::string font: { "liberation/LiberationSans-Regular.ttf",
            "liberation/LiberationSans-Bold.ttf",
            "liberation/LiberationSans-Italic.ttf",
            "liberation/LiberationSans-BoldItalic.ttf"}) {
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
    context_ = Rocket::Core::CreateContext("default", Rocket::Core::Vector2i(width_, height_));
    rocket_render_interface_->register_context(context_, &window_.scene());

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

    for(std::string font_dir: paths) {
        if(os::path::exists(os::path::join(font_dir, filename))) {
            return os::path::join(font_dir, filename);
        }
    }

    throw IOError("Unable to locate font: " + filename);
}

void Interface::update(float dt) {
    context_->Update();
}

void Interface::render() {
    context_->Render();
}

Interface::~Interface() {
    if(rocket_render_interface_) {
        rocket_render_interface_->unregister_context(context_);
    }

    context_->RemoveReference();

    Rocket::Core::Shutdown();
}

}
}
