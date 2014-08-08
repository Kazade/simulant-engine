#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <mutex>

namespace Rocket {
namespace Core {

class Context;
class ElementDocument;

}
}

namespace kglt {

class RocketImpl;

namespace ui {

class ElementImpl {
public:
    ElementImpl(RocketImpl& rocket_impl, Rocket::Core::Element* elem):
        rocket_impl_(rocket_impl),
        elem_(elem),
        text_(nullptr) {

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

private:
    RocketImpl& rocket_impl_;
    Rocket::Core::Element* elem_;
    Rocket::Core::ElementText* text_;
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
