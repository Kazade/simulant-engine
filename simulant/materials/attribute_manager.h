#pragma once

#include <functional>
#include <unordered_map>

#include "../vertex_data.h"

namespace smlt {

enum ShaderAvailableAttributes {
    SP_ATTR_VERTEX_POSITION,
    SP_ATTR_VERTEX_NORMAL,
    SP_ATTR_VERTEX_TEXCOORD0,
    SP_ATTR_VERTEX_TEXCOORD1,
    SP_ATTR_VERTEX_TEXCOORD2,
    SP_ATTR_VERTEX_TEXCOORD3,
    SP_ATTR_VERTEX_DIFFUSE,
    SP_ATTR_VERTEX_SPECULAR
};

VertexAttributeType convert(ShaderAvailableAttributes attr);

}

namespace std {
    using smlt::ShaderAvailableAttributes;

    template<>
    struct hash<ShaderAvailableAttributes> {
        size_t operator()(const ShaderAvailableAttributes& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };

}

namespace smlt {

class AttributeManager {
public:
    void register_auto(ShaderAvailableAttributes attr, const std::string &var_name);

    std::string variable_name(ShaderAvailableAttributes attr_name) const {
        auto it = auto_attributes_.find(attr_name);
        if(it == auto_attributes_.end()) {
            throw std::logic_error("Specified attribute is not registered");
        }

        return (*it).second;
    }

    bool uses_auto(ShaderAvailableAttributes attr) const {
        return auto_attributes_.find(attr) != auto_attributes_.end();
    }

    const std::unordered_map<ShaderAvailableAttributes, std::string>& auto_attributes() const {
        return auto_attributes_;
    }

private:
    std::unordered_map<ShaderAvailableAttributes, std::string> auto_attributes_;
};

}


