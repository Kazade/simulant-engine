#include "material_object.h"
#include "material_property.h"
#include "material_property.inl"
#include "material_property_registry.h"

namespace smlt {

std::string MaterialPropertyValue::shader_variable() const {
    return property_->name;
}

std::string MaterialPropertyValue::name() const {
    return property_->name;
}

bool MaterialPropertyValue::is_custom() const {
    return property_->is_custom;
}

MaterialPropertyType MaterialPropertyValue::type() const {
    return property_->type;
}

/* Find the object in the entry list, and either mark it as
 * empty, or, if it's the final entry then reduce the entry
 * count */
MaterialPropertyValue *MaterialProperty::value(MaterialObject *obj) {
    auto entry = &entries[obj->object_id_];
    if(!entry->object) {
        return &entries[0].value;
    } else {
        return &entry->value;
    }
}

void MaterialProperty::release_entry(const MaterialObject *object) {
    /* Wipe the object pointer to make the slot unset */
    entries[object->object_id_].object = nullptr;
}

}
