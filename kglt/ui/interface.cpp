#ifndef __ANDROID__
    #include <SDL2/SDL_rwops.h>
#else
    #include <SDL_rwops.h>
#endif

#include <kazmath/mat4.h>
#include <thread>

#include "../loader.h"
#include <kazbase/os/path.h>
#include "../window_base.h"
#include "../camera.h"
#include "../render_sequence.h"
#include "../utils/gl_error.h"

#include "interface.h"
#include "ui_private.h"

#ifdef KGLT_GL_VERSION_2X
#include "../buffer_object.h"
#endif

namespace kglt {
namespace ui {

Interface::Interface(WindowBase &window):
    window_(window) {

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

void Interface::render(const Mat4 &projection_matrix) {
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
