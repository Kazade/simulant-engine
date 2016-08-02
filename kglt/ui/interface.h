#ifndef INTERFACE_H
#define INTERFACE_H

#include <memory>
#include <unordered_map>
#include <kazbase/unicode.h>
#include "../generic/managed.h"
#include"../types.h"
#include "../loadable.h"
#include "element.h"

namespace kglt {

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

    ElementList append(const unicode& tag) {
        std::vector<Element> new_elements;
        for(Element& e: elements_) {
            new_elements.push_back(e.append(tag));
        }

        return ElementList(new_elements);
    }

    bool is(const std::string& selector) {
        if(selector != ":visible") {
            throw ValueError("Unsupported selector: " + selector);
        }

        for(Element& e: elements_) {
            if(e.is_visible()) {
                return true;
            }
        }

        return false;
    }

    ElementList add_class(const unicode& cl) {
        for(Element& e: elements_) {
            e.add_class(cl);
        }

        return *this;
    }

    ElementList set_event_callback(const unicode& event_type, std::function<bool (Event)> func) {
        for(Element& e: elements_) {
            e.set_event_callback(event_type, func);
        }

        return *this;
    }

    ElementList remove_class(const unicode& cl) {
        for(Element& e: elements_) {
            e.remove_class(cl);
        }

        return *this;
    }

    void css(const std::string& property, const std::string& value) {
        for(Element& e: elements_) {
            e.css(property, value);
        }
    }

    void attr(const std::string& property, const std::string& value) {
        for(Element& e: elements_) {
            e.attr(property, value);
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

    void show(const std::string& transition="") {
        for(Element& e: elements_) {
            e.show(transition);
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

    void remove_children() {
        for(Element& e: elements_) {
            e.remove_children();
        }
    }

    void html(const unicode& rml) {
        for(Element& e: elements_) {
            e.inner_rml(rml);
        }
    }

private:
    std::vector<Element> elements_;
};


class Interface :
    public Managed<Interface>,
    public Loadable {

public:
    Interface(WindowBase& window);
    ~Interface();

    uint16_t width() const;
    uint16_t height() const;

    void set_dimensions(uint16_t width, uint16_t height);

    bool init();
    void update(float dt);
    void render(const Mat4& projection_matrix);

    ElementList append(const unicode& tag);
    void set_styles(const std::string& stylesheet_content);
    ElementList _(const unicode& selector);

    Mat4 projection_matrix() const { return projection_matrix_; }

    void load_font(const unicode& ttf_file);

    WindowBase* window() { return &window_; }

private:    
    void set_projection_matrix(const Mat4& mat) { projection_matrix_ = mat; }
    std::vector<unicode> find_fonts();

    unicode locate_font(const unicode& filename);

    WindowBase& window_;
    Mat4 projection_matrix_;
};

}
}

#endif // INTERFACE_H
