#ifndef MATERIAL_PROPERTY_INL
#define MATERIAL_PROPERTY_INL

#include "material_object.h"

namespace smlt {

template<typename T>
void MaterialProperty::set_value(const MaterialObject* object, const T& value) {
    auto entry = &entries[object->object_id_];
    entry->object = object;
    entry->value = MaterialPropertyValue(this, value);
    /* Mark this entry as "overridden" */
    entry->is_set = true;

    /* This was the root object */
    if(object->object_id_ == 0) {
        /* When we're setting the root value, we set the same
         * value on all objects that haven't been set */
        for(auto i = 1u; i < _S_ARRAY_LENGTH(entries); ++i) {
            auto& e = entries[i];
            if(e.object && !e.is_set) {
                e.value = MaterialPropertyValue(this, value);
            }
        }
    }
}

}

#endif
