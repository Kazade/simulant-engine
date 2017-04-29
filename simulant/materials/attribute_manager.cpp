#include "attribute_manager.h"

namespace smlt {

void AttributeManager::register_auto(ShaderAvailableAttributes attr, const std::string &var_name) {
    auto_attributes_[attr] = var_name;
}

VertexAttributeType convert(ShaderAvailableAttributes attr) {
    /*
     * We have basically the same list of attributes in VertexData but...
     * - If VertexData looks at this list, then it ties the VertexData (which is pretty modular and can be rendered with GL1)
     *   to the gpu_program stuff which we don't want.
     * - Conversely, shader attributes aren't technically tied to vertex data so using the VertexAttribute type from here doesn't
     *   make sense either. For now I've just made this conversion function until I decide on something better
     */
    switch(attr) {
        case SP_ATTR_VERTEX_POSITION: return VERTEX_ATTRIBUTE_TYPE_POSITION;
        case SP_ATTR_VERTEX_NORMAL: return VERTEX_ATTRIBUTE_TYPE_NORMAL;
        case SP_ATTR_VERTEX_TEXCOORD0: return VERTEX_ATTRIBUTE_TYPE_TEXCOORD0;
        case SP_ATTR_VERTEX_TEXCOORD1: return VERTEX_ATTRIBUTE_TYPE_TEXCOORD1;
        case SP_ATTR_VERTEX_TEXCOORD2: return VERTEX_ATTRIBUTE_TYPE_TEXCOORD2;
        case SP_ATTR_VERTEX_TEXCOORD3: return VERTEX_ATTRIBUTE_TYPE_TEXCOORD3;
        case SP_ATTR_VERTEX_DIFFUSE: return VERTEX_ATTRIBUTE_TYPE_DIFFUSE;
        case SP_ATTR_VERTEX_SPECULAR: return VERTEX_ATTRIBUTE_TYPE_SPECULAR;
    default:
        return VERTEX_ATTRIBUTE_TYPE_EMPTY;
    }

}

}
