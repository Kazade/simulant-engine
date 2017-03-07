#pragma once

#include <functional>
#include <unordered_map>

namespace smlt {

/*
  Automatic uniforms that are set by the renderer
*/
enum ShaderAvailableAuto {
    SP_AUTO_MODELVIEW_PROJECTION_MATRIX,
    SP_AUTO_VIEW_MATRIX,
    SP_AUTO_MODELVIEW_MATRIX,
    SP_AUTO_PROJECTION_MATRIX,
    SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX,
    SP_AUTO_MATERIAL_DIFFUSE,
    SP_AUTO_MATERIAL_SPECULAR,
    SP_AUTO_MATERIAL_AMBIENT,
    SP_AUTO_MATERIAL_SHININESS,
    SP_AUTO_MATERIAL_TEX_MATRIX0,
    SP_AUTO_MATERIAL_TEX_MATRIX1,
    SP_AUTO_MATERIAL_TEX_MATRIX2,
    SP_AUTO_MATERIAL_TEX_MATRIX3,
    SP_AUTO_MATERIAL_TEX_MATRIX4,
    SP_AUTO_MATERIAL_TEX_MATRIX5,
    SP_AUTO_MATERIAL_TEX_MATRIX6,
    SP_AUTO_MATERIAL_TEX_MATRIX7,
    SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS,
    SP_AUTO_MATERIAL_POINT_SIZE,
    SP_AUTO_LIGHT_GLOBAL_AMBIENT,
    SP_AUTO_LIGHT_POSITION,
    SP_AUTO_LIGHT_DIRECTION,
    SP_AUTO_LIGHT_DIFFUSE,
    SP_AUTO_LIGHT_SPECULAR,
    SP_AUTO_LIGHT_AMBIENT,
    SP_AUTO_LIGHT_CONSTANT_ATTENUATION,
    SP_AUTO_LIGHT_LINEAR_ATTENUATION,
    SP_AUTO_LIGHT_QUADRATIC_ATTENUATION

    //TODO: cameras(?)
};

}

namespace std {
    using smlt::ShaderAvailableAuto;

    template<>
    struct hash<ShaderAvailableAuto> {
        size_t operator()(const ShaderAvailableAuto& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };
}

namespace smlt {

class UniformManager {
public:
    bool uses_auto(ShaderAvailableAuto uniform) const {
        return auto_uniforms_.find(uniform) != auto_uniforms_.end();
    }

    std::string auto_variable_name(ShaderAvailableAuto auto_name) const {
        auto it = auto_uniforms_.find(auto_name);
        if(it == auto_uniforms_.end()) {
            throw std::logic_error("Specified auto is not registered");
        }

        return (*it).second;
    }

    void register_auto(ShaderAvailableAuto uniform, const std::string& var_name);
    const std::unordered_map<ShaderAvailableAuto, std::string>& auto_uniforms() const {
        return auto_uniforms_;
    }

private:
    std::unordered_map<ShaderAvailableAuto, std::string> auto_uniforms_;
};


}


