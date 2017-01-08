//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

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

ElementImpl::ElementImpl(Interface *interface, TiXmlElement *element):
    interface_(interface),
    element_(element) {

    if(element->ValueStr() == "window") {
        set_background_colour(Colour(0, 0, 0, 0.5));
        set_text_colour(Colour::SLATE_GREY);
    } else {
        set_background_colour(Colour::SLATE_GREY);
        set_text_colour(Colour::WHITE);
    }
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
