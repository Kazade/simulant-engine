#include "interface.h"
#include "ui_private.h"
#include "../window_base.h"

namespace smlt {
namespace ui {

const std::string HIDDEN = "1";
const std::string VISIBLE = "0";

Element::Element(std::shared_ptr<ElementImpl> impl):
    impl_(impl) {

}

std::string Element::name() const {
    return impl_->name();
}

Element Element::append_row() {
    return impl_->append_row();
}

Element Element::append_label(const unicode &label) {
    return impl_->append_label(label);
}

Element Element::append_progress_bar() {
    return impl_->append_progress_bar();
}

void Element::set_text(const unicode& text) {
    impl_->set_text(text);
}

const unicode Element::text() const {
    return impl_->text();
}

void Element::remove_class(const unicode &cl) {
    for(auto cls: cl.split(" ")) {
        impl_->remove_class(cls.encode());
    }
}

void Element::add_class(const unicode &cl) {
    for(auto cls: cl.split(" ")) {
        impl_->add_class(cls.encode());
    }
}

void Element::remove_children() {
    impl_->remove_children();
}

void Element::inner_rml(const unicode& rml) {
    impl_->inner_rml(rml);
}

bool Element::is_dead() const { return impl_->is_dead(); }

std::string Element::css(const std::string& property) const {
    return impl_->css(property);
}

void Element::add_css(const std::string& property, const std::string& value) {
    impl_->add_css(property, value);
}

void Element::set_attr(const std::string& property, const std::string& value) {
    impl_->set_attr(property, value);
}

std::string Element::attr(const std::string& property) {
    return impl_->attr(property);
}

std::string Element::id() const {
    return impl_->id();
}

void Element::set_id(const std::string& id) {
    impl_->set_id(id);
}

void Element::scroll_to_bottom() {
    impl_->scroll_to_bottom();
}

void Element::show(const std::string& transition) {
    /*if(transition == "fade") {
        css("opacity", "0"); // Make transparent

        // Wrap a new refrence to the impl
        Element copy(impl_);

        // Trigger a task to increase the opacity in the background
        idle().add([copy]() mutable -> bool {
            auto current = unicode(copy.css("opacity")).to_float();
            current += 0.01;
            if(current >= 1.0) {
                copy.css("opacity", "1");
                // Make visible
                return false;
            }
            copy.css("opacity", std::to_string(current));
            return true;
        });
    }
    // Make visible
    css("visibility", "visible"); */
}

bool Element::is_visible() const {
    auto visibility = css("visibility");
    return visibility != HIDDEN;
}

void Element::set_event_callback(EventType event_type, EventCallback func) {
    impl_->set_event_callback(event_type, func);
}

float Element::left() const {
    return impl_->left();
}

float Element::top() const {
    return impl_->top();
}

float Element::width() const {
    return impl_->width();
}

float Element::height() const {
    return impl_->height();
}

}

}
