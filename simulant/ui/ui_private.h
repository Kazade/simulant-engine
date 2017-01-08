/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UI_PRIVATE_H
#define UI_PRIVATE_H

#include <mutex>
#include <unordered_map>
#include <tinyxml.h>

#include "element.h"


namespace smlt {

namespace ui {

class Interface;

class ElementImpl {
public:
    ElementImpl(Interface* interface, TiXmlElement* element);

    ~ElementImpl() {

    }

    std::string name() const;

    void set_text(const unicode& text);
    const unicode text() const {
        return element_->GetText();
    }

    void add_class(const std::string& cl) {
        const std::string* ret = element_->Attribute(std::string("class"));
        std::string current;

        if(ret) {
            current = *ret;
        }

        if(current.find(cl) == std::string::npos) {
            current += " " + cl;
            element_->SetAttribute("class", current);
        }
    }

    void remove_class(const std::string& cl) {
        assert(0 && "Not Implemented");
    }

    std::string css(const std::string& property) {
        if(styles_.count(property)) {
            return styles_[property];
        } else {
            // FIXME: Recurse up the dom
        }

        return "";
    }

    void add_css(const std::string& property, const std::string& value) {
        styles_[property] = value;
    }

    void set_attr(const std::string& property, const std::string& value) {
        element_->SetAttribute(property, value);
    }

    std::string attr(const std::string& property) const {
        const std::string* value = element_->Attribute(property);
        if(value) {
            return *value;
        }

        return "";
    }

    void set_id(const std::string& id) {
        element_->SetAttribute("id", id);
    }

    std::string id() const {
        return this->attr("id");
    }

    void scroll_to_bottom() {

    }

    void remove_children();

    void inner_rml(const unicode& rml);

    void set_event_callback(EventType event_type, EventCallback func);

    bool is_dead() const { return !interface_; }

    Element append_row();
    Element append_label(const unicode& text);
    Element append_progress_bar();

    void set_background_colour(const smlt::Colour& colour);
    void set_border_colour(const smlt::Colour& colour);
    void set_text_colour(const smlt::Colour& colour);
    void set_border_width(const float width);
    void set_border_radius(const float radius);
    void set_text_alignment(TextAlignment alignment);
    void set_padding(float padding);

    Colour background_colour() const {
        return Colour::from_hex_string(styles_.at("background-color"));
    }

    Colour text_colour() const {
        return Colour::from_hex_string(styles_.at("color"));
    }

private:    
    friend class Interface;

    std::map<EventType, EventCallback> event_callbacks_;
    Interface* interface_ = nullptr;
    TiXmlElement* element_ = nullptr;

    std::unordered_map<std::string, std::string> styles_;

    ui::Element append(const std::string& tag);
};


}
}

#endif // UI_PRIVATE_H
