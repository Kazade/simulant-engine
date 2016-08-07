#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <mutex>
#include <unordered_map>
#include <kazbase/exceptions.h>
#include <tinyxml.h>

#include "element.h"


namespace kglt {

namespace ui {

class Interface;

class ElementImpl {
public:
    ElementImpl(Interface* interface, TiXmlElement* element):
        interface_(interface),
        element_(element) {

    }

    ~ElementImpl() {

    }

    std::string name() const;

    void set_text(const unicode& text);
    const unicode text() const {
        return element_->GetText();
    }

    void add_class(const std::string& cl) {
        const std::string* ret = element_->Attribute(std::string("class"));
        std::string current;

        if(ret) {
            current = *ret;
        }

        if(current.find(cl) == std::string::npos) {
            current += " " + cl;
            element_->SetAttribute("class", current);
        }
    }

    void remove_class(const std::string& cl) {
        assert(0 && "Not Implemented");
    }

    std::string css(const std::string& property) {
        if(styles_.count(property)) {
            return styles_[property];
        } else {
            // FIXME: Recurse up the dom
        }

        return "";
    }

    void css(const std::string& property, const std::string& value) {
        styles_[property] = value;
    }

    void attr(const std::string& property, const std::string& value) {
        element_->SetAttribute(property, value);
    }

    std::string attr(const std::string& property) const {
        const std::string* value = element_->Attribute(property);
        if(value) {
            return *value;
        }

        return "";
    }

    void id(const std::string& id) {
        element_->SetAttribute("id", id);
    }

    std::string id() const {
        return this->attr("id");
    }

    void scroll_to_bottom() {

    }

    void remove_children() {
    }

    void inner_rml(const unicode& rml);
    kglt::ui::Element append(const unicode& tag);

    void set_event_callback(const unicode& event_type, std::function<bool (Event)> func);

    float left() const;
    float top() const;
    float width() const;
    float height() const;

private:
    std::unordered_map<unicode, std::function<bool (Event)> > event_callbacks_;
    Interface* interface_ = nullptr;
    TiXmlElement* element_ = nullptr;

    std::unordered_map<std::string, std::string> styles_;
};


}
}

#endif // UI_PRIVATE_H
