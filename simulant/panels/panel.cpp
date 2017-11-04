#include "panel.h"

namespace smlt {

void Panel::on_key_down(const KeyEvent &evt) {
    if(evt.keyboard_code == activation_key_) {
        if(is_active()) {
            deactivate();
        } else {
            activate();
        }
    }
}

}
