#include "shortcuts.h"
#include "../scene.h"
#include "label.h"
#include "types.h"
#include "../ui.h"

namespace kglt {
namespace ui {

Label& return_new_label(Scene& scene) {
    LabelID new_label = scene.ui().new_label();
    return scene.ui().label(new_label);
}

}
}
