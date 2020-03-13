#include "material_object.h"
#include "material_property.h"
#include "material_property.inl.h"
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
MaterialPropertyValue *MaterialProperty::value(const MaterialObject *obj) {
    return &entries[obj->object_id_].value;
}

const MaterialPropertyValue *MaterialProperty::value(const MaterialObject *obj) const {
    return &entries[obj->object_id_].value;
}

void MaterialProperty::init_entry(const MaterialObject* object) {
    assert(object->object_id_ > 0);
    entries[object->object_id_].object = object;
    entries[object->object_id_].value = entries[0].value;
}

void MaterialProperty::release_entry(const MaterialObject *object) {
    assert(object->object_id_ > 0);

    /* Wipe the object pointer to make the slot unset */
    entries[object->object_id_].object = nullptr;

    /* Restore the default value */
    entries[object->object_id_].is_set = false;
    entries[object->object_id_].value.set_value(false);
}

}
