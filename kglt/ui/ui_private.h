#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <mutex>
#include <unordered_map>
#include "element.h"

#include <Rocket/Core/EventListener.h>

namespace Rocket {
namespace Core {

class Context;
class ElementDocument;

}
}

namespace kglt {

class RocketImpl;

namespace ui {

const std::vector<unicode> STANDARD_EVENTS = {
    "click", "mousemove", "show", "hide", "resize", "scroll", "focus", "blur"
};

class ElementImpl : public Rocket::Core::EventListener {
public:
    ElementImpl(RocketImpl& rocket_impl, Rocket::Core::Element* elem):
        rocket_impl_(rocket_impl),
        elem_(elem),
        text_(nullptr) {


        for(auto evt: STANDARD_EVENTS) {
            elem_->AddEventListener(evt.encode().c_str(), this);
        }
    }

    ~ElementImpl() {
        for(auto evt: STANDARD_EVENTS) {
            elem_->RemoveEventListener(evt.encode().c_str(), this);
        }
    }

    void set_text(const unicode& text);
    const unicode text() const {
        if(!text_) {
            return unicode("");
        }

        return unicode((char16_t*)text_->GetText().CString());
    }

    void add_class(const std::string& cl) {
        elem_->SetClass(cl.c_str(), true);
    }

    void remove_class(const std::string& cl) {
        elem_->SetClass(cl.c_str(), false);
    }

    std::string css(const std::string& property) {
        return elem_->GetProperty<Rocket::Core::String>(property.c_str()).CString();
    }

    void css(const std::string& property, const std::string& value) {
        elem_->SetProperty(property.c_str(), value.c_str());
    }

    void attr(const std::string& property, const std::string& value) {
        elem_->SetAttribute(property.c_str(), value.c_str());
    }

    void id(const std::string& id) {
        elem_->SetId(id.c_str());
    }

    void scroll_to_bottom() {
        elem_->SetScrollTop(elem_->GetScrollHeight());
    }

    kglt::ui::Element append(const unicode& tag);

    void set_event_callback(const unicode& event_type, std::function<bool ()> func);

private:
    RocketImpl& rocket_impl_;
    Rocket::Core::Element* elem_;
    Rocket::Core::ElementText* text_;

    void ProcessEvent(Rocket::Core::Event& event);

    std::unordered_map<unicode, std::function<bool ()> > event_callbacks_;
};

}

struct RocketImpl {
    RocketImpl():
        context_(nullptr),
        document_(nullptr) {}

    Rocket::Core::Context* context_;
    Rocket::Core::ElementDocument* document_;
    std::recursive_mutex mutex_;
};
}

#endif // UI_PRIVATE_H
