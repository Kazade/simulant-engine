#define DEFINE_STAGENODEPOOL
#include "../nodes/stage_node_pool.h"

#include "../stage.h"
#include "panel.h"
#include "../application.h"

namespace smlt {

bool Panel::init() {
    auto node_pool = get_app()->stage_node_pool.get();
    stage_ = std::make_shared<Stage>(nullptr, node_pool, smlt::PARTITIONER_NULL);
    return true;
}

void Panel::clean_up() {
    stage_.reset();
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
