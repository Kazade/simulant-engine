#ifndef __ANDROID__
    #include <SDL2/SDL_rwops.h>
#else
    #include <SDL_rwops.h>
#endif

#include <thread>
#include <kazmath/mat4.h>
#include <kazbase/os/path.h>

#include "../loader.h"
#include "../ui_stage.h"
#include "../window_base.h"
#include "../camera.h"
#include "../render_sequence.h"
#include "../utils/gl_error.h"

#include "interface.h"
#include "ui_private.h"

namespace kglt {
namespace ui {

Interface::Interface(WindowBase &window, UIStage *owner):
    window_(window),
    stage_(owner) {

}

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
        "kglt/fonts/fontawesome/fontawesome-webfont.ttf",
        "kglt/fonts/probe/PROBE_10PX_TTF.ttf",
        "kglt/fonts/probe/PROBE_10PX_TTF Bold.ttf",
    };

    std::vector<unicode> results;
    for(unicode font: FONT_PATHS) {
        const unicode path = window_.resource_locator->locate_file(font);
        results.push_back(path);
    }

    return results;
}

bool Interface::init() {
    nk_init_default(&nk_ctx_, nullptr);
    return true;
}

void Interface::load_font(const unicode &ttf_file) {

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


    return window_.resource_locator->locate_file(filename).encode();

/*
    for(std::string font_dir: paths) {


        if(os::path::exists(os::path::join(font_dir, filename))) {
            return os::path::join(font_dir, filename);
        }
    }

    throw IOError("Unable to locate font: " + filename);*/
}

void Interface::update(float dt) {

}

void xml_iterator(
        const TiXmlNode* el,
        std::function<void (const TiXmlNode*, bool)> callback) {

    for(const TiXmlNode* node = el->FirstChild(); node; node = node->NextSibling()) {
        callback(node, false);
        xml_iterator(node, callback);
        callback(node, true);
    }
}

void Interface::render(CameraPtr camera, Viewport viewport) {
    auto callback = [this](const TiXmlNode* node, bool after) {
        bool before = !after;

        const TiXmlElement* element_node = node->ToElement();
        if(!element_node) {
            return; //Not an element
        }

        Element element(element_impls_.at((TiXmlElement*)element_node));

        if(element.name() == "window") {
            if(before) {
                std::string title = element.attr("title");

                struct nk_rect bounds;
                bounds.x = element.css("left").empty() ? 0 : std::stoi(element.css("left"));
                bounds.y = element.css("top").empty() ? 0 : std::stoi(element.css("top"));
                bounds.w = element.css("width").empty() ? window_.width() : std::stoi(element.css("width"));
                bounds.h = element.css("height").empty() ? window_.height() : std::stoi(element.css("height"));

                nk_begin(&nk_ctx_, &nk_layout_, title.c_str(), bounds, 0);
            } else {
                nk_end(&nk_ctx_);
            }
        } else if(element.name() == "row") {

        } else if(element.name() == "button") {

        } else {
            L_WARN_ONCE(_u("Ignoring unknown element: {0}").format(element.name()));
        }
    };
    xml_iterator(document_.FirstChild(), callback);

    send_to_renderer(camera, viewport);

}

void Interface::send_to_renderer(CameraPtr camera, Viewport viewport) {
    // Now to actually render everything
    const struct nk_draw_command *cmd;
    struct nk_buffer vbuf, ebuf;

    /* fill converting configuration */
    struct nk_convert_config config;
    memset(&config, 0, sizeof(config));
    config.global_alpha = 1.0f;
    config.shape_AA = NK_ANTI_ALIASING_ON;
    config.line_AA = NK_ANTI_ALIASING_ON;
    config.circle_segment_count = 22;
    config.curve_segment_count = 22;
    config.arc_segment_count = 22;
    config.null = nk_device_.null;

    /* convert shapes into vertexes */
    nk_buffer_init_default(&vbuf);
    nk_buffer_init_default(&ebuf);
    nk_convert(&nk_ctx_, &nk_device_.cmds, &vbuf, &ebuf, &config);

    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_2F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;

    const nk_byte* vertices = (const nk_byte*) nk_buffer_memory_const(&vbuf);
    const nk_draw_index *offset = (const nk_draw_index*) nk_buffer_memory_const(&ebuf);

    VertexData vertex_data(spec);

    auto vertex_size = (sizeof(float) * 4 + sizeof(uint32_t));

    const nk_byte* current = vertices;
    for(uint32_t i = 0; i < vbuf.size / vertex_size; ++i) {
        float x = ((float*)current)[0];
        float y = ((float*)current)[1];
        float u = ((float*)current)[2];
        float v = ((float*)current)[3];

        current += sizeof(float) * 4;
        uint32_t colour = ((uint32_t*)current)[0];
        current += sizeof(uint32_t);

        vertex_data.position(x, y);
        vertex_data.tex_coord0(u, v);
        vertex_data.diffuse(kglt::Colour::WHITE); //FIXME
        vertex_data.move_next();
    }
    vertex_data.done();

    std::vector<UIRenderable> renderables;

    auto& renderer = this->window_.renderer;

    nk_draw_foreach(cmd, &nk_ctx_, &nk_device_.cmds) {
        renderables.push_back(UIRenderable(vertex_data, MaterialID(cmd->texture.id)));

        UIRenderable& renderable = renderables.back();

        // FIXME: clipping?
        for(uint32_t i = 0; i < cmd->elem_count; ++i) {
            renderable.index_data->index(offset[i]);
        }
        renderable.index_data->done();
        offset += cmd->elem_count;
    }

    for(auto& renderable: renderables) {
        MaterialPass* pass = stage_->resources->material(renderable.material_id())->first_pass().get();
        auto render_group = renderer->new_render_group(&renderable, pass);
        renderer->render(
            camera,
            true, // Render group changed
            &render_group,
            &renderable,
            pass,
            nullptr,
            kglt::Colour::WHITE,
            kglt::batcher::Iteration(0)
        );
    }
}

void Interface::set_dimensions(uint16_t width, uint16_t height) {
}

uint16_t Interface::width() const {
}

uint16_t Interface::height() const {
}

ElementList Interface::append(const unicode &tag) {
    TiXmlElement* element = new TiXmlElement(tag.lstrip("<").rstrip(">").encode());
    document_.LinkEndChild(element);

    element_impls_[element] = std::make_shared<ElementImpl>(this, element);

    return ElementList({Element(element_impls_[element])});
}

ElementList Interface::_(const unicode &selectors) {
    std::vector<Element> elements;

    for(auto selector: selectors.split(",")) {
        selector = selector.strip();

        for(auto& p: this->element_impls_) {
            if(selector.starts_with("#")) {
                if(p.second->id() == selector.lstrip("#").encode()) {
                    elements.push_back(Element(p.second));
                }
            } else if(selector.starts_with(".")) {
                if(p.second->attr("class").find(selector.lstrip(".").encode()) != std::string::npos) {
                    elements.push_back(Element(p.second));
                }
            }
        }
    }
    return ElementList(elements);
}

void Interface::set_styles(const std::string& stylesheet_content) {
}

Interface::~Interface() {
}

}
}
