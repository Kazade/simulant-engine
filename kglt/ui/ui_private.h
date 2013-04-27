#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

namespace Rocket {
namespace Core {

class Context;
class ElementDocument;

}
}

namespace kglt {
namespace ui {

class ElementImpl {
public:
    ElementImpl(Rocket::Core::Element* elem):
        elem_(elem),
        text_(nullptr) {

    }

    void set_text(const unicode& text) {
        /*
         *  Element objects simply wrap the underlying Rocket::Core::Element*
         *  and so when we call set text we need to look and see if there is a
         *  text node for this element. If not, we create it.
         *
         *  Then we set the text.
         */
        if(!text_) {
            Rocket::Core::ElementList elements;
            elem_->GetElementsByTagName(elements, "#text");
            if(elements.empty()) {
                text_ = elem_->GetOwnerDocument()->CreateTextNode(text.encode().c_str());
                elem_->AppendChild(text_);
            } else {
                text_ = dynamic_cast<Rocket::Core::ElementText*>(elements[0]);
                text_->SetText(text.encode().c_str());
            }

        } else {
            text_->SetText(text.encode().c_str());
        }
    }

    const unicode text() const {
        if(!text_) {
            return unicode("");
        }

        return unicode((char16_t*)text_->GetText().CString());
    }

    void add_class(const std::string& cl) {
        elem_->SetClass(cl.c_str(), true);
    }

    std::string css(const std::string& property) {
        return elem_->GetProperty<Rocket::Core::String>(property.c_str()).CString();
    }

    void css(const std::string& property, const std::string& value) {
        elem_->SetProperty(property.c_str(), value.c_str());
    }

private:
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
};
}

#endif // UI_PRIVATE_H
