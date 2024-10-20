#include "gl1x_vertex_buffer_data.h"

namespace smlt {

/* This format matches GL1Vertex but bear in mind that the colors stored
 * in that struct are *input* colors and a separate array is used for rendering
 * and the format used there does not match this one */
const VertexFormat GL1XVertexBufferData::format =
    VertexFormatBuilder()
        .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT, 32) // 32-byte aligned
        .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_RGBA,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_NORMAL, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_TANGENT, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .add(VERTEX_ATTR_NAME_BITANGENT, VERTEX_ATTR_ARRANGEMENT_XYZ,
             VERTEX_ATTR_TYPE_FLOAT)
        .build();
}
