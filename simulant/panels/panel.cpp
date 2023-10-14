#define DEFINE_STAGENODEPOOL
#include "../nodes/stage_node_pool.h"

#include "../stage.h"
#include "panel.h"
#include "../application.h"

namespace smlt {

bool Panel::on_init() {

    return true;
}

void Panel::on_clean_up() {
}

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
