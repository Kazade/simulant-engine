#include <mutex>
#include <thread>
#include <iostream>
#include <kazbase/unicode.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementText.h>
#include <Rocket/Core/ElementDocument.h>
#include "ui_private.h"

namespace kglt {
namespace ui {

Element ElementImpl::append(const unicode& tag) {
    std::lock_guard<std::recursive_mutex> lck(rocket_impl_.mutex_);

    Rocket::Core::Element* elem = elem_->GetOwnerDocument()->CreateElement(tag.encode().c_str());
    elem_->AppendChild(elem);

    Element result = Element(
        std::shared_ptr<ElementImpl>(
            new ElementImpl(rocket_impl_, elem)
        )
    );

    return result;
}

void ElementImpl::set_event_callback(const unicode& event_type, std::function<bool ()> func) {
    event_callbacks_[event_type] = func;
}

void ElementImpl::ProcessEvent(Rocket::Core::Event& event) {
    unicode event_type = event.GetType().CString();
    std::cout << event_type << std::endl;
    auto it = event_callbacks_.find(event_type);
    if(it != event_callbacks_.end()) {
        bool ret = (*it).second();
        if(ret) {
            event.StopPropagation();
        }
    }
}

void ElementImpl::set_text(const unicode& text) {
    /*
     *  Element objects simply wrap the underlying Rocket::Core::Element*
     *  and so when we call set text we need to look and see if there is a
     *  text node for this element. If not, we create it.
     *
     *  Then we set the text.
     */
    if(!text_) {
        std::lock_guard<std::recursive_mutex> lck(rocket_impl_.mutex_);

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
        std::lock_guard<std::recursive_mutex> lck(rocket_impl_.mutex_);
        text_->SetText(text.encode().c_str());
    }
}

}
}
