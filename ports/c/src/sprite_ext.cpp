#include "simulant/c/sprite_ext.h"

#include "simulant/nodes/sprite.h"

extern "C" {

smlt_spritesheet_attrs_t* smlt_spritesheet_attrs_create_default(void) {
    return reinterpret_cast<smlt_spritesheet_attrs_t*>(new smlt::SpritesheetAttrs());
}

} // extern "C"
