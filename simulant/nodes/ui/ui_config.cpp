#include "ui_config.h"
#include "../../colour.h"
#include "../../application.h"

namespace smlt {
namespace ui {

const char* DEFAULT_FONT_FAMILY = "__DEFAULT_FAMILY__";
const Px DEFAULT_FONT_SIZE = Px(-1);

const Colour UIConfig::ALICE_BLUE = Colour(0.9375, 0.96875, 0.99609375, 1.0);
const Colour UIConfig::LIGHT_GREY = Colour(0.82421875, 0.82421875, 0.82421875, 1.0);
const Colour UIConfig::DODGER_BLUE = Colour(0.1171875, 0.5625, 0.99609375, 1.0);

Px Px::operator*(const Rem &rhs) const {
    return Px(int(float(value) * rhs.value));
}

Rem::operator Px() const {
    auto def_size = UIConfig().font_size_;
    auto app = smlt::get_app();
    if(app) {
        def_size = Px(app->config->ui.font_size);
    }

    return Px(def_size.value * value);
}

}
}
