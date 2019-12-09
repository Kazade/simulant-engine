
#include "material_property.h"
#include "material_property_registry.h"

namespace smlt {

std::string MaterialPropertyValue::shader_variable() const {
    return registry_->property(id_)->name;
}

std::string MaterialPropertyValue::name() const {
    return registry_->property(id_)->name;
}

bool MaterialPropertyValue::is_custom() const {
    return registry_->property(id_)->is_custom;
}

MaterialPropertyType MaterialPropertyValue::type() const {
    return registry_->property(id_)->type;
}

}
