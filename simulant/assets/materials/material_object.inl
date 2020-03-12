#ifndef MATERIAL_OBJECT_INL
#define MATERIAL_OBJECT_INL

#include "material_object.h"

namespace smlt {

template<typename T>
void MaterialObject::set_property_value(const MaterialPropertyID& id, const T& value) {
    registry_->properties_[id - 1].set_value(this, value);
}

template<typename T>
void MaterialObject::set_property_value(const std::string& name, const T& value) {
    set_property_value(registry_->find_property_id(name), value);
}


}

#endif
