#pragma once

#include "material_object.h"

namespace smlt {

template<typename T>
void MaterialProperty::set_value(const MaterialObject* object, const T& value) {
    auto entry = &entries[object->object_id_];
    entry->object = object;
    entry->value = MaterialPropertyValue(this, value);
}

}
