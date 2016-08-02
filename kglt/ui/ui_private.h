#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <mutex>
#include <unordered_map>
#include <kazbase/exceptions.h>

#include "element.h"


namespace kglt {

namespace ui {

class ElementImpl {
public:
    ElementImpl(void* something) {

    }

    ~ElementImpl() {

    }

    void set_text(const unicode& text);
    const unicode text() const {
    }

    void add_class(const std::string& cl) {
    }

    void remove_class(const std::string& cl) {
    }

    std::string css(const std::string& property) {
    }

    void css(const std::string& property, const std::string& value) {
    }

    void attr(const std::string& property, const std::string& value) {
    }

    void id(const std::string& id) {
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
};


}
}

#endif // UI_PRIVATE_H
