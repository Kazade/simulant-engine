
#include "loading.h"

namespace kglt {
namespace screens {

Loading::Loading(Scene &scene):
    scene_(scene),
    is_active_(false) {

}

void Loading::activate() {
    is_active_ = true;
}

void Loading::deactivate() {
    is_active_ = false;
}

void Loading::update(float dt) {
    if(!is_active()) return;
}

bool Loading::is_active() const {
    return is_active_;
}

}
}
