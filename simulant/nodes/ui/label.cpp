#include "label.h"

namespace smlt {
namespace ui {

Label::Label(WidgetID id, UIManager *owner, UIConfig *config):
    Widget(id, owner, config) {

    set_resize_mode(config->label_resize_mode_);
}


}
}
