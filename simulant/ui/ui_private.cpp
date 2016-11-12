#include <mutex>
#include <thread>
#include <iostream>
#include "../deps/kazlog/kazlog.h"
#include "ui_private.h"
#include "interface.h"
#include "../utils/unicode.h"

namespace smlt {
namespace ui {


Element ElementImpl::append_row() {
    return append("<row>");
}

Element ElementImpl::append_label(const unicode &text) {
    auto e = append("<label>");
    e.set_text(text);
    return e;
}

Element ElementImpl::append_progress_bar() {
    auto e = append("<progress_bar>");
    return e;
}

Element ElementImpl::append(const std::string &tag) {
    TiXmlElement* element = new TiXmlElement(_u(tag).lstrip("<").rstrip(">").encode());
    element_->LinkEndChild(element);
    interface_->element_impls_[element] = std::make_shared<ElementImpl>(interface_, element);
    return Element(interface_->element_impls_[element]);
}

std::string ElementImpl::name() const {
    return element_->ValueStr();
}

void ElementImpl::set_event_callback(EventType event_type, EventCallback func) {
    event_callbacks_[event_type] = func;
}

void ElementImpl::set_text(const unicode& text) {
    element_->RemoveChild(element_->FirstChild());

    TiXmlText* text_node = new TiXmlText(text.encode());
    element_->LinkEndChild(text_node);
}

void ElementImpl::remove_children() {

    std::function<void (TiXmlNode*)> walk = [&](TiXmlNode* node) {
        for(auto child = node->FirstChild(); child; child = child->NextSibling()) {
            /* This takes some explaining...
             *
             * Element objects hold a shared_ptr to an ElementImpl which in turn wraps a TiXmlElement.
             *
             * What we do is update the ElementImpl and set its TiXmlElement to null. Then we ignore
             * ElementImpl with a null TiXMLElement for all things
             */

            if(child->FirstChild()) {
                walk(child);
            }

            auto it = interface_->element_impls_.find((TiXmlElement*) child);
            if(it != interface_->element_impls_.end()) {
                it->second->element_ = nullptr;
                interface_->element_impls_.erase(it);
            }


            node->RemoveChild(child);
        }
    };

    walk(element_);
}

void ElementImpl::inner_rml(const unicode& rml) {

}

void ElementImpl::set_background_colour(const smlt::Colour& colour) {
    styles_["background-color"] = colour.to_hex_string();
}

void ElementImpl::set_border_colour(const smlt::Colour& colour) {
    styles_["border-color"] = colour.to_hex_string();
}

void ElementImpl::set_text_colour(const smlt::Colour& colour) {
    styles_["color"] = colour.to_hex_string();
}

void ElementImpl::set_border_width(const float width) {
    styles_["border-width"] = std::to_string(width) + "px";
}

void ElementImpl::set_border_radius(const float radius) {
    styles_["border-radius"] = std::to_string(radius) + "px";
}

void ElementImpl::set_text_alignment(TextAlignment alignment) {
    styles_["text-align"] = (alignment == TEXT_ALIGNMENT_CENTRE) ? "center" : (alignment == TEXT_ALIGNMENT_LEFT) ? "left" : "right";
}

void ElementImpl::set_padding(float padding) {
    styles_["padding"] = std::to_string(padding) + "px";
}

}
}
