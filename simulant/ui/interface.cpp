//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __ANDROID__
    #include <SDL2/SDL_rwops.h>
#else
    #include <SDL_rwops.h>
#endif

#include <thread>
#include <iostream>

#include "../loader.h"
#include "../overlay.h"
#include "../window_base.h"
#include "../camera.h"
#include "../render_sequence.h"
#include "../utils/gl_error.h"

#include "../resource_manager.h"
#include "interface.h"
#include "ui_private.h"

namespace smlt {
namespace ui {

const VertexSpecification UIRenderable::VERTEX_SPECIFICATION = {
    VERTEX_ATTRIBUTE_2F, //Position
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_2F, //Texcoord0
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_4F //Diffuse
};

UIRenderable::UIRenderable(HardwareBuffer::ptr& index_buffer, HardwareBuffer* vertex_buffer, MaterialID material):
    vertex_buffer_(vertex_buffer),
    index_buffer_(std::move(index_buffer)),
    material_id_(material) {


}

Interface::Interface(WindowBase &window, Overlay *owner):
    window_(window),
    stage_(owner) {

    TiXmlElement* root_element = new TiXmlElement("window");
    document_.LinkEndChild(root_element);

    element_impls_[root_element] = std::make_shared<ElementImpl>(this, root_element);
    root_element_ = std::make_shared<Element>(element_impls_[root_element]);
}

std::vector<unicode> Interface::find_fonts() {
    /*
     * Unfortunately, because Android doesn't easily let you list folders in a portable way
     * we have to hard code the list of fonts. Bummer.
     */

    const std::vector<unicode> FONT_PATHS = {
        "smlt/fonts/opensans/OpenSans-BoldItalic.ttf",
        "smlt/fonts/opensans/OpenSans-Bold.ttf",
        "smlt/fonts/opensans/OpenSans-ExtraBoldItalic.ttf",
        "smlt/fonts/opensans/OpenSans-ExtraBold.ttf",
        "smlt/fonts/opensans/OpenSans-Italic.ttf",
        "smlt/fonts/opensans/OpenSans-LightItalic.ttf",
        "smlt/fonts/opensans/OpenSans-Light.ttf",
        "smlt/fonts/opensans/OpenSans-Regular.ttf",
        "smlt/fonts/opensans/OpenSans-SemiboldItalic.ttf",
        "smlt/fonts/opensans/OpenSans-Semibold.ttf",
        "smlt/fonts/ubuntu/Ubuntu-BI.ttf",
        "smlt/fonts/ubuntu/Ubuntu-B.ttf",
        "smlt/fonts/ubuntu/Ubuntu-C.ttf",
        "smlt/fonts/ubuntu/Ubuntu-LI.ttf",
        "smlt/fonts/ubuntu/Ubuntu-L.ttf",
        "smlt/fonts/ubuntu/Ubuntu-MI.ttf",
        "smlt/fonts/ubuntu/Ubuntu-M.ttf",
        "smlt/fonts/ubuntu/Ubuntu-RI.ttf",
        "smlt/fonts/ubuntu/Ubuntu-R.ttf",
        "smlt/fonts/ubuntu/UbuntuMono-BI.ttf",
        "smlt/fonts/ubuntu/UbuntuMono-B.ttf",
        "smlt/fonts/ubuntu/UbuntuMono-RI.ttf",
        "smlt/fonts/ubuntu/UbuntuMono-R.ttf",
        "smlt/fonts/fontawesome/fontawesome-webfont.ttf",
        "smlt/fonts/probe/PROBE_10PX_TTF.ttf",
        "smlt/fonts/probe/PROBE_10PX_TTF Bold.ttf",
    };

    std::vector<unicode> results;
    for(unicode font: FONT_PATHS) {
        const unicode path = window_.resource_locator->locate_file(font);
        results.push_back(path);
    }

    return results;
}

bool Interface::init() {
    auto null_material_id = stage_->assets->new_material_from_texture(
        stage_->assets->default_texture_id(),
        GARBAGE_COLLECT_NEVER
    );

    auto null_material = stage_->assets->material(null_material_id);
    null_material->first_pass()->set_cull_mode(CULL_MODE_NONE);
    null_material->first_pass()->set_blending(BLEND_ALPHA);
    null_material->first_pass()->set_depth_test_enabled(false);

    // Load a material which renders a single white pixel texture
    nk_device_.null_tex = null_material_id;

    // Set the null texture to use this material
    nk_device_.null.texture = nk_handle_id((int) nk_device_.null_tex.value());
    nk_device_.null.uv.x = nk_device_.null.uv.y = 0;

    nk_font_atlas_init_default(&nk_font_);
    nk_font_atlas_begin(&nk_font_);

    auto font_path = locate_font("simulant/fonts/opensans/OpenSans-Regular.ttf");
    auto data = window()->resource_locator->read_file(font_path)->str();
    struct nk_font* font = nk_font_atlas_add_from_memory(
        &nk_font_,
        (void*) data.c_str(), data.size(),
        13, 0
    );

    int w, h;
    const nk_byte* image = (nk_byte*) nk_font_atlas_bake(&nk_font_, &w, &h, NK_FONT_ATLAS_RGBA32);

    TextureID font_tex = stage_->assets->new_texture();
    TexturePtr tex = stage_->assets->texture(font_tex);
    tex->resize(w, h);
    tex->data().assign(image, image + (w * h * 4));
    tex->upload(
        MIPMAP_GENERATE_NONE,
        TEXTURE_WRAP_CLAMP_TO_EDGE,
        TEXTURE_FILTER_LINEAR,
        true
    );

    auto font_material_id = stage_->assets->new_material_from_texture(font_tex, GARBAGE_COLLECT_NEVER);
    auto font_material = stage_->assets->material(font_material_id);
    font_material->first_pass()->set_cull_mode(CULL_MODE_NONE);
    font_material->first_pass()->set_blending(BLEND_ALPHA);
    font_material->first_pass()->set_depth_test_enabled(false);

    nk_device_.font_tex = font_material_id;
    nk_font_atlas_end(&nk_font_, nk_handle_id((int)nk_device_.font_tex.value()), nullptr);
    nk_init_default(&nk_ctx_, &font->handle);
    nk_buffer_init_default(&nk_device_.cmds);

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


        if(kfs::path::exists(kfs::path::join(font_dir, filename))) {
            return kfs::path::join(font_dir, filename);
        }
    }

    throw IOError("Unable to locate font: " + filename);*/
}

void Interface::update(float dt) {

}

void xml_iterator(
        const TiXmlNode* el,
        std::function<void (const TiXmlNode*, bool)> callback) {

    callback(el, false);
    for(const TiXmlNode* node = el->FirstChild(); node; node = node->NextSibling()) {        
        xml_iterator(node, callback);
    }
    callback(el, true);
}


nk_color nk_color_from(const Colour& colour) {
    return nk_rgba_f(colour.r, colour.g, colour.b, colour.a);
}

void Interface::render(CameraPtr camera, Viewport viewport) {
    auto callback = [this](const TiXmlNode* node, bool after) {
        bool before = !after;

        const TiXmlElement* element_node = node->ToElement();

        if(!element_node) {
            return; //Not an element
        }

        Element element(element_impls_.at((TiXmlElement*)element_node));
        auto element_name = element.name();
        if(element_name == "window") {
            if(before) {
                std::string title = element.attr("title");

                struct nk_rect bounds;
                bounds.x = element.css("left").empty() ? 0 : std::stoi(element.css("left"));
                bounds.y = element.css("top").empty() ? 0 : std::stoi(element.css("top"));
                bounds.w = element.css("width").empty() ? window_.width() : std::stoi(element.css("width"));
                bounds.h = element.css("height").empty() ? window_.height() : std::stoi(element.css("height"));

                auto background_colour = nk_color_from(element.background_colour());
                auto text_colour = nk_color_from(element.text_colour());

                nk_ctx_.style.window.background = background_colour;
                nk_ctx_.style.window.fixed_background = nk_style_item_color(background_colour);

                nk_begin(&nk_ctx_, title.c_str(), bounds, 0);
            } else {
                nk_end(&nk_ctx_);
            }
        } else if(element_name == "row") {
            if(before) {
                int child_count = 0;
                for(auto child = element_node->FirstChild(); child; child = child->NextSibling()) {
                    child_count++;
                }

                nk_layout_row_dynamic(&nk_ctx_, 20, child_count + 1);
            } else {

            }
        } else if(element_name == "label") {
            if(before) {
                auto text = element.text().encode();
                nk_label(&nk_ctx_, text.c_str(), NK_LEFT);
            }

        } else if(element_name == "progress_bar") {
            if(before) {
                nk_prog(&nk_ctx_, 50, 100, 1);
            }
        } else if(element_name == "button") {

        } else {
            L_WARN_ONCE(_F("Ignoring unknown element: {0}").format(element.name()));
        }
    };

    xml_iterator(document_.RootElement(), callback);
    send_to_renderer(camera, viewport);

}

struct nk_smlt_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};


void Interface::send_to_renderer(CameraPtr camera, Viewport viewport) {
    // Now to actually render everything
    const struct nk_draw_command *cmd;
    struct nk_buffer vbuf, ebuf;

    /* fill converting configuration */
    struct nk_convert_config config;
    memset(&config, 0, sizeof(config));

    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_smlt_vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_smlt_vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_smlt_vertex, col)},
        {NK_VERTEX_LAYOUT_END}
    };

    config.vertex_layout = vertex_layout;
    config.vertex_size = sizeof(struct nk_smlt_vertex);
    config.vertex_alignment = NK_ALIGNOF(struct nk_smlt_vertex);

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

    const nk_smlt_vertex* vertices = (const nk_smlt_vertex*) ((const nk_byte*) nk_buffer_memory_const(&vbuf));
    const nk_draw_index *offset = (const nk_draw_index*) nk_buffer_memory_const(&ebuf);

    VertexData vertex_data(UIRenderable::VERTEX_SPECIFICATION);

    auto vertex_size = (sizeof(float) * 4 + sizeof(uint32_t));

    const nk_smlt_vertex* current = vertices;
    for(uint32_t i = 0; i < vbuf.size / vertex_size; ++i) {
        float x = current->position[0];
        float y = current->position[1];
        float u = current->uv[0];
        float v = current->uv[1];


        smlt::Colour rgba(
            float(current->col[0]) / 255.0f,
            float(current->col[1]) / 255.0f,
            float(current->col[2]) / 255.0f,
            float(current->col[3]) / 255.0f
        );

        vertex_data.position(x, y);
        vertex_data.tex_coord0(u, v);
        vertex_data.diffuse(rgba);
        vertex_data.move_next();

        current++;
    }
    vertex_data.done();

    if(!shared_vertex_buffer_) {
        shared_vertex_buffer_ = window_.renderer->hardware_buffers->allocate(
            vertex_data.data_size(),
            HARDWARE_BUFFER_VERTEX_ATTRIBUTES
        );
    } else {
        shared_vertex_buffer_->resize(vertex_data.data_size());
    }
    shared_vertex_buffer_->upload(vertex_data);

    std::vector<std::shared_ptr<UIRenderable>> renderables;

    auto& renderer = this->window_.renderer;

    nk_draw_foreach(cmd, &nk_ctx_, &nk_device_.cmds) {
        if(!cmd->elem_count) continue;

        HardwareBuffer::ptr index_buffer = renderer->hardware_buffers->allocate(
            cmd->elem_count * sizeof(Index),
            HARDWARE_BUFFER_VERTEX_ARRAY_INDICES
        );

        renderables.push_back(std::make_shared<UIRenderable>(
            index_buffer, shared_vertex_buffer_.get(), MaterialID(cmd->texture.id))
        );

        auto renderable = renderables.back();
        IndexData index_data;

        // FIXME: clipping?
        for(uint32_t i = 0; i < cmd->elem_count; ++i) {
            index_data.index(offset[i]);
        }
        index_data.done();
        renderable->index_buffer()->upload(index_data);

        offset += cmd->elem_count;
    }

    nk_clear(&nk_ctx_);
    nk_buffer_free(&vbuf);
    nk_buffer_free(&ebuf);

    for(auto& renderable: renderables) {
        auto material = stage_->assets->material(renderable->material_id());
        MaterialPass* pass = material->first_pass().get();
        auto render_group = renderer->new_render_group(renderable.get(), pass);
        renderer->render(
            camera,
            true, // Render group changed
            &render_group,
            renderable.get(),
            pass,
            nullptr,
            smlt::Colour::WHITE,
            smlt::batcher::Iteration(0)
        );
    }


}

void Interface::set_dimensions(uint16_t width, uint16_t height) {
}

uint16_t Interface::width() const {
}

uint16_t Interface::height() const {
}

ElementList Interface::append_row() {
    return ElementList(std::vector<Element>{root_element_->append_row()});
}

ElementList Interface::find(const unicode &selectors) {
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

void Interface::add_css(const std::string& property, const std::string& value) {
    root_element_->add_css(property, value);
}

void Interface::set_styles(const std::string& stylesheet_content) {
}

Interface::~Interface() {
}

}
}
