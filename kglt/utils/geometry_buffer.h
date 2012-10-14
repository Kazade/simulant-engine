#ifndef GEOMETRY_BUFFER_H
#define GEOMETRY_BUFFER_H

#include "glee/GLee.h"
#include <cstdint>
#include <tr1/memory>
#include <vector>

#include "../types.h"

namespace kglt {

/**
 * @brief The GeometryBuffer class
 * Holds a bunch of geometry ready for batching or rendering
 */

struct GeometryBufferEntry {
    kmVec3 position;
    kmVec2 texcoord_1;
    kmVec3 normal;
    Colour diffuse;
};

class GeometryBuffer {
public:
    typedef std::tr1::shared_ptr<GeometryBuffer> ptr;

    GeometryBuffer(MeshArrangement arrangement, uint32_t attributes);
    void resize(uint32_t vertex_count);
    void set_vertex(uint32_t vertex, const GeometryBufferEntry& values);

    MeshArrangement arrangement() const;
    uint32_t attributes() const;
    uint32_t stride() const;
    MaterialID material() const;

    int32_t offset(VertexAttribute attr); ///< Offset into the vertex data that stores the attribute
    uint32_t count() const;

    GLuint vbo();

    bool has_attribute(VertexAttribute attr) const { return (attributes_ & attr) == attr; }

    void set_material(MaterialID mat) { material_ = mat; }

private:
    MeshArrangement arrangement_;
    uint32_t attributes_;
    uint32_t stride_;
    MaterialID material_;

    std::vector<float> buffer_;

    mutable bool is_dirty_;
    GLuint vertex_buffer_;

    uint32_t floats_per_vertex() const { return stride() / sizeof(float); }
};

}

#endif // GEOMETRY_BUFFER_H
