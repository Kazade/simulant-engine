#include "label.h"

namespace kglt {
namespace extra {
namespace ui {

Label::Label(Interface* interface, LabelID id):
    Widget(*interface),
    generic::Identifiable<LabelID>(id) {
}

void Label::set_text(const std::string& text) {
    text_ = text;
}

}
}
}
