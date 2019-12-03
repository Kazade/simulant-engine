#include "material_property_registry.h"

namespace smlt {

BlendType blend_type_from_name(const std::string &v) {
    if(v == "alpha") {
        return BLEND_ALPHA;
    } else if(v == "add") {
        return BLEND_ADD;
    } else if(v == "colour") {
        return BLEND_COLOUR;
    } else if(v == "modulate") {
        return BLEND_MODULATE;
    } else if(v == "one_one_minus_alpha") {
        return BLEND_ONE_ONE_MINUS_ALPHA;
    } else {
        L_WARN(_F("Unrecognised blend value {0}").format(v));
        return BLEND_NONE;
    }
}

MaterialPropertyID MaterialPropertyRegistry::find_property_id(const std::string& name) {
    auto& it = std::find_if(
                properties_.begin(),
                properties_.end(), [&name](const MaterialProperty& i) -> bool {
        return i.name == name;
    }
    );

    if(it != properties_.end()) {
        return it->id;
    } else {
        return MATERIAL_PROPERTY_ID_INVALID;
    }
}

MaterialProperty* MaterialPropertyRegistry::property(MaterialPropertyID id) {
    assert(id > 0);
    return &properties_[id - 1];
}

void MaterialPropertyRegistry::register_all_builtin_properties() {
    material_ambient_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, AMBIENT_PROPERTY, Vec4(1, 1, 1, 1));
    material_diffuse_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, DIFFUSE_PROPERTY, Vec4(1, 1, 1, 1));
    material_specular_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, SPECULAR_PROPERTY, Vec4(1, 1, 1, 1));
    material_shininess_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, SHININESS_PROPERTY, 0.0f);

    diffuse_map_id_ =register_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, DIFFUSE_MAP_PROPERTY, TextureUnit());
    light_map_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, LIGHT_MAP_PROPERTY, TextureUnit());
    normal_map_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, NORMAL_MAP_PROPERTY, TextureUnit());
    specular_map_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, SPECULAR_MAP_PROPERTY, TextureUnit());

    blend_func_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_INT, BLEND_FUNC_PROPERTY, (int) BLEND_NONE);
    cull_mode_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_INT, CULL_MODE_PROPERTY, (int) CULL_MODE_BACK_FACE);
    colour_material_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_INT, COLOUR_MATERIAL_PROPERTY, (int) COLOUR_MATERIAL_NONE);

    depth_test_enabled_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, DEPTH_TEST_ENABLED_PROPERTY, true);
    // register_builtin_property(DEPTH_FUNC_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, DEPTH_FUNC_LEQUAL);

    depth_write_enabled_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, DEPTH_WRITE_ENABLED_PROPERTY, true);
    shade_model_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_INT, SHADE_MODEL_PROPERTY, (int) SHADE_MODEL_SMOOTH);
    polygon_mode_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_INT, POLYGON_MODE_PROPERTY, (int) POLYGON_MODE_FILL);

    lighting_enabled_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, LIGHTING_ENABLED_PROPERTY, false);
    texturing_enabled_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, TEXTURING_ENABLED_PROPERTY, true);
    point_size_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, POINT_SIZE_PROPERTY, 1.0f);

    register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_POSITION_PROPERTY, Vec4());
    register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_AMBIENT_PROPERTY, Vec4(1, 1, 1, 1));
    register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_DIFFUSE_PROPERTY, Vec4(1, 1, 1, 1));
    register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_SPECULAR_PROPERTY, Vec4(1, 1, 1, 1));

    register_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, LIGHT_CONSTANT_ATTENUATION_PROPERTY, 0.0f);
    register_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, LIGHT_LINEAR_ATTENUATION_PROPERTY, 0.0f);
    register_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, LIGHT_QUADRATIC_ATTENUATION_PROPERTY, 0.0f);

    register_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, MODELVIEW_MATRIX_PROPERTY, Mat4());
    register_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, MODELVIEW_PROJECTION_MATRIX_PROPERTY, Mat4());
    register_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, PROJECTION_MATRIX_PROPERTY, Mat4());
    register_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, VIEW_MATRIX_PROPERTY, Mat4());
    register_builtin_property(MATERIAL_PROPERTY_TYPE_MAT3, INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY, Mat3());
}

void MaterialPropertyRegistry::register_object(MaterialObject* obj, MaterialObjectType type) {
    if(type == MATERIAL_OBJECT_TYPE_ROOT) {
        assert(!root_);
        obj->object_id_ = 0;
        root_ = obj;
    } else {
        obj->object_id_ = object_values_.size();
        object_values_.push_back(MaterialObjectValues());
    }

    // Make sure the values are active
    object_values_[obj->object_id].is_active = true;
}

MaterialPropertyValue* MaterialPropertyRegistry::property_value(MaterialObject* obj, MaterialPropertyID id) {
    auto& values = object_values_[obj->object_id];
    assert(values.is_active);
    return &values.values.at(id);
}

}
