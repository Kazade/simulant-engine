#ifndef INTERFACE_H
#define INTERFACE_H

#include <tr1/memory>

#include "../kazbase/unicode.h"
#include "../generic/managed.h"
#include"../types.h"
#include "../loadable.h"
#include "element.h"

namespace kglt {

struct RocketImpl;
class WindowBase;

namespace ui {

class Interface;


class ElementList {
public:
    ElementList(const std::vector<Element>& elements):
        elements_(elements) {}

    void text(const unicode& text) {
        for(Element& e: elements_) {
            e.text(text);
        }
    }

    void add_class(const std::string& cl) {
        for(Element& e: elements_) {
            e.add_class(cl);
        }
    }

    void css(const std::string& property, const std::string& value) {
        for(Element& e: elements_) {
            e.css(property, value);
        }
    }

    Element& operator[](uint32_t i) {
        return elements_[i];
    }

    const Element& operator[](uint32_t i) const {
        return elements_[i];
    }

    std::vector<Element>::iterator begin() { return elements_.begin(); }
    std::vector<Element>::iterator end() { return elements_.end(); }

    bool empty() const { return elements_.empty(); }

    void show() {
        for(Element& e: elements_) {
            e.show();
        }
    }

    void hide() {
        for(Element& e: elements_) {
            e.hide();
        }
    }

    void id(const std::string& id) {
        for(Element& e: elements_) {
            e.id(id);
        }
    }

    void scroll_to_bottom() {
        for(Element& e: elements_) {
            e.scroll_to_bottom();
        }
    }

private:
    std::vector<Element> elements_;
};

class Interface :
    public Managed<Interface>,
    public Loadable {

public:
    Interface(WindowBase& window, uint32_t width_in_pixels, uint32_t height_in_pixels);
    ~Interface();

    uint16_t width_in_pixels() const { return width_; }
    uint16_t height_in_pixels() const { return height_; }

    RocketImpl* impl() { return impl_.get(); }

    bool init();
    void update(float dt);
    void render();

    Element append(const std::string& tag);
    void set_styles(const std::string& stylesheet_content);
    ElementList _(const std::string& selector);

private:    
    std::string locate_font(const std::string& filename);

    WindowBase& window_;

    uint32_t width_;
    uint32_t height_;

    std::unique_ptr<RocketImpl> impl_;
};

}
}

#endif // INTERFACE_H
