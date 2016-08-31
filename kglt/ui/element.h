#ifndef ELEMENT_H
#define ELEMENT_H

#include <string>
#include <memory>

#include "../idle_task_manager.h"
#include "../utils/unicode.h"

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

    std::string name() const;

    void text(const unicode& content);
    const unicode text() const;

    void add_class(const unicode& cl);
    void remove_class(const unicode &cl);

    std::string css(const std::string& property) const;
    void css(const std::string& property, const std::string& value);
    void attr(const std::string& property, const std::string& value);
    std::string attr(const std::string& property);

    void show(const std::string& transition="");
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

    void remove_children();
    void inner_rml(const unicode& rml);
private:
    std::shared_ptr<ElementImpl> impl_;
};

}
}

#endif // ELEMENT_H
