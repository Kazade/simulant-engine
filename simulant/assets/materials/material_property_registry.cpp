#include "material_property_registry.h"
#include "material_object.h"

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

MaterialPropertyRegistry::MaterialPropertyRegistry() {
    registered_objects_.push_back(nullptr); // Always make room for the root object

    register_all_builtin_properties();
}

MaterialPropertyID MaterialPropertyRegistry::find_property_id(const std::string& name) const {
    auto it = std::find_if(
                properties_.begin(),
                properties_.end(), [&name](const MaterialProperty& i) -> bool {
        return i.name == name;
    });

    if(it != properties_.end()) {
        return it->id;
    } else {
        return MATERIAL_PROPERTY_ID_INVALID;
    }
}

const MaterialProperty* MaterialPropertyRegistry::property(MaterialPropertyID id) const {
    assert(id > 0);
    return &properties_[id - 1];
}

std::size_t MaterialPropertyRegistry::registered_material_object_count() const {
    return std::count_if(
        registered_objects_.begin(),
        registered_objects_.end(),
        [](MaterialObject* i) -> bool {
            return i != nullptr;
        }
    );
}

void MaterialPropertyRegistry::copy_from(const MaterialPropertyRegistry& rhs) {
    if(&rhs == this) return;

    root_ = nullptr;
    registered_objects_.clear();
    registered_objects_.push_back(nullptr);

    properties_ = rhs.properties_;

    for(auto& prop: properties_) {
        prop.default_value.registry_ = this;
    }

    rebuild_texture_properties();
    rebuild_custom_properties();

    material_ambient_id_ = rhs.material_ambient_id_;
    material_diffuse_id_ = rhs.material_diffuse_id_;
    material_specular_id_ = rhs.material_specular_id_;
    material_shininess_id_ = rhs.material_shininess_id_;
    diffuse_map_id_ = rhs.diffuse_map_id_;
    specular_map_id_ = rhs.specular_map_id_;
    light_map_id_ = rhs.light_map_id_;
    normal_map_id_ = rhs.normal_map_id_;

    blending_enabled_id_ = rhs.blending_enabled_id_;
    texturing_enabled_id_ = rhs.texturing_enabled_id_;
    lighting_enabled_id_ = rhs.lighting_enabled_id_;
    depth_test_enabled_id_ = rhs.depth_test_enabled_id_;
    depth_write_enabled_id_ = rhs.depth_write_enabled_id_;

    shade_model_id_ = rhs.shade_model_id_;
    cull_mode_id_ = rhs.cull_mode_id_;
    polygon_mode_id_ = rhs.polygon_mode_id_;
    point_size_id_ = rhs.point_size_id_;
    colour_material_id_ = rhs.colour_material_id_;
    blend_func_id_ = rhs.blend_func_id_;
}

void MaterialPropertyRegistry::register_all_builtin_properties() {
    material_ambient_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, AMBIENT_PROPERTY, Vec4(1, 1, 1, 1));
    material_diffuse_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, DIFFUSE_PROPERTY, Vec4(1, 1, 1, 1));
    material_specular_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, SPECULAR_PROPERTY, Vec4(1, 1, 1, 1));
    material_shininess_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, SHININESS_PROPERTY, 0.0f);

    diffuse_map_id_ = register_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, DIFFUSE_MAP_PROPERTY, TextureUnit());
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
    auto it = std::find(registered_objects_.begin(), registered_objects_.end(), obj);
    if(it != registered_objects_.end()) {
        return;
    }

    /* FIXME: Look for empty slots in the registered_objects array */
    if(type == MATERIAL_OBJECT_TYPE_ROOT) {
        assert(!root_);
        obj->object_id_ = 0;
        root_ = obj;
        registered_objects_[0] = obj;
    } else {
        obj->object_id_ = registered_objects_.size();
        registered_objects_.push_back(obj);
    }

    obj->registry_ = this;
    for(auto& prop: obj->property_values_) {
        prop.value.registry_ = this;
    }
}

void MaterialPropertyRegistry::unregister_object(MaterialObject* obj) {
    if(obj == root_) {
        root_ = registered_objects_[0] = nullptr;
    } else {
        registered_objects_[obj->object_id_] = nullptr;
    }
}

}
