#include <mutex>
#include <thread>
#include <iostream>
#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementText.h>
#include <Rocket/Core/ElementDocument.h>
#include "ui_private.h"

namespace kglt {
namespace ui {

Element ElementImpl::append(const unicode& tag) {
    std::lock_guard<std::recursive_mutex> lck(rocket_impl_->mutex_);

    Rocket::Core::Element* elem = elem_->GetOwnerDocument()->CreateElement(tag.encode().c_str());
    elem_->AppendChild(elem);

    Element result = Element(rocket_impl_->document_->get_impl_for_element(elem));

    return result;
}

float ElementImpl::left() const {
    return elem_->GetAbsoluteLeft();
}

float ElementImpl::top() const {
    return elem_->GetAbsoluteTop();
}

float ElementImpl::width() const {
    return elem_->GetOffsetWidth();
}

float ElementImpl::height() const {
    return elem_->GetOffsetHeight();
}

void ElementImpl::set_event_callback(const unicode& event_type, std::function<bool (Event)> func) {
    event_callbacks_[event_type] = func;
}

void ElementImpl::ProcessEvent(Rocket::Core::Event& event) {
    unicode event_type = event.GetType().CString();

    ui::Event evt;
    if(event_type == "touchover" || event_type == "touchout" || event_type == "touchdown" || event_type == "touchup") {
        evt.type = EVENT_TYPE_TOUCH;
        evt.touch.finger_id = event.GetParameter<int>("finger_id", 0);
    }

    auto it = event_callbacks_.find(event_type);
    if(it != event_callbacks_.end()) {
        bool ret = (*it).second(evt);
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

    if(!rocket_impl_) {
        throw ValueError("rocket_impl_ has not been set");
    }

    if(!text_) {
        std::lock_guard<std::recursive_mutex> lck(rocket_impl_->mutex_);

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
        std::lock_guard<std::recursive_mutex> lck(rocket_impl_->mutex_);
        text_->SetText(text.encode().c_str());
    }
}


CustomDocument::CustomDocument(const Rocket::Core::String& tag):
    Rocket::Core::ElementDocument(tag) {

}

void find_all_children(Rocket::Core::Element* parent, std::vector<Rocket::Core::Element*> children) {
    for(int i = 0; i < parent->GetNumChildren(); ++i) {
        children.push_back(parent->GetChild(i));
        find_all_children(children.back(), children);
    }
}

void CustomDocument::set_impl(RocketImpl* impl) {
    impl_ = impl;

    std::vector<Rocket::Core::Element*> children;
    find_all_children(this->impl_->document_, children);

    for(auto child: children) {
        OnChildAdd(child);
    }

    for(auto p: element_impls_) {
        p.second->_set_rocket_impl(impl);
    }
}

std::shared_ptr<ElementImpl> CustomDocument::get_impl_for_element(Rocket::Core::Element* element) {
    return element_impls_.at(element);
}


void CustomDocument::OnChildAdd(Rocket::Core::Element* element) {
    /*
     *  When we create the CustomDocument itself, it will call OnChildAdd... on itself!
     *  At this point we don't have a RocketImpl instance, so we have to set it to NULL
     *  on the element impl. Then, we call set_impl (above) which will set the impl on all elements.
     *
     *  This is very hacky, but I can't think how else to do it!
     */

    auto it = element_impls_.find(element);
    if(it == element_impls_.end()) {
        if(!impl_ && element != this) {
            throw ValueError("impl_ has not been set");
        }
        element_impls_[element] = std::make_shared<ElementImpl>(impl_, element);
    } else {
        return;
    }

    Rocket::Core::ElementDocument::OnChildAdd(element);
}

void CustomDocument::OnChildRemove(Rocket::Core::Element* element) {
    Rocket::Core::ElementDocument::OnChildRemove(element);

    auto it = element_impls_.find(element);
    if(it != element_impls_.end()) {
        element_impls_.erase(it);
    } else {
        L_WARN("ChildRemove called on an element we didn't know about");
    }
}


}
}
