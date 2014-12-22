#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <mutex>
#include <unordered_map>
#include <kazbase/exceptions.h>

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
    "click", "mousedown", "mouseup", "mousemove", "mouseover", "mouseout",
    "show", "hide", "resize", "scroll", "focus", "blur",
    "touchdown", "touchup", "touchmove", "touchout", "touchover"
};

class ElementImpl : public Rocket::Core::EventListener {
public:
    ElementImpl(RocketImpl* rocket_impl, Rocket::Core::Element* elem):
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

    void set_event_callback(const unicode& event_type, std::function<bool (Event)> func);

    void _set_rocket_impl(RocketImpl* impl) { rocket_impl_ = impl; }

    float left() const;
    float top() const;
    float width() const;
    float height() const;
private:
    RocketImpl* rocket_impl_ = nullptr;
    Rocket::Core::Element* elem_ = nullptr;
    Rocket::Core::ElementText* text_ = nullptr;

    void ProcessEvent(Rocket::Core::Event& event);

    std::unordered_map<unicode, std::function<bool (Event)> > event_callbacks_;
};

class CustomDocument : public Rocket::Core::ElementDocument {
public:
    CustomDocument(const Rocket::Core::String& tag);

    void set_impl(RocketImpl* impl);

    std::shared_ptr<ElementImpl> get_impl_for_element(Rocket::Core::Element* element);

protected:
    virtual void OnChildAdd(Rocket::Core::Element* element);

    virtual void OnChildRemove(Rocket::Core::Element* element);
private:
    RocketImpl* impl_ = nullptr;
    std::unordered_map<Rocket::Core::Element*, std::shared_ptr<ElementImpl>> element_impls_;
};

}

struct RocketImpl {
    RocketImpl():
        context_(nullptr),
        document_(nullptr) {}

    Rocket::Core::Context* context_;
    kglt::ui::CustomDocument* document_;
    std::recursive_mutex mutex_;
};
}

#endif // UI_PRIVATE_H
