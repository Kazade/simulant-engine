#ifndef ELEMENT_H
#define ELEMENT_H

#include <string>
#include <memory>
#include <kazbase/unicode.h>

namespace kglt {
namespace ui {

class ElementImpl;

struct TouchEvent {
    int finger_id;
};

enum EventType {
    EVENT_TYPE_NONE,
    EVENT_TYPE_TOUCH,
};

union Event {
    EventType type = EVENT_TYPE_NONE;
    TouchEvent touch;
};

class Element {
public:
    Element(std::shared_ptr<ElementImpl> impl);

    void text(const unicode& content);
    const unicode text() const;

    void add_class(const unicode& cl);
    void remove_class(const unicode &cl);

    std::string css(const std::string& property) const;
    void css(const std::string& property, const std::string& value);
    void attr(const std::string& property, const std::string& value);

    void show() {
        css("visibility", "visible");
    }

    void hide() {
        css("visibility", "hidden");
    }

    void id(const std::string& id);

    void scroll_to_bottom();

    bool is_visible() const;

    Element append(const unicode& tag);

    void set_event_callback(const unicode& event_type, std::function<bool (Event)> func);

    float left() const;
    float top() const;
    float width() const;
    float height() const;

private:
    std::shared_ptr<ElementImpl> impl_;

};

}
}

#endif // ELEMENT_H
