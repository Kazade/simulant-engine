#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementText.h>
#include <Rocket/Core/ElementDocument.h>

#include "interface.h"
#include "ui_private.h"

namespace kglt {
namespace ui {

const std::string HIDDEN = "1";
const std::string VISIBLE = "0";

Element::Element(std::shared_ptr<ElementImpl> impl):
    impl_(impl) {

}

Element Element::append(const std::string& tag) {
    return impl_->append(tag);
}

void Element::text(const unicode& text) {
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

std::string Element::css(const std::string& property) const {
    return impl_->css(property);
}

void Element::css(const std::string& property, const std::string& value) {
    impl_->css(property, value);
}

void Element::attr(const std::string& property, const std::string& value) {
    impl_->attr(property, value);
}

void Element::id(const std::string& id) {
    impl_->id(id);
}

void Element::scroll_to_bottom() {
    impl_->scroll_to_bottom();
}

bool Element::is_visible() const {
    auto visibility = css("visibility");
    return visibility != HIDDEN;
}

}

}
