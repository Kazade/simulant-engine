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

class GeometryBuffer {
public:
    typedef std::tr1::shared_ptr<GeometryBuffer> ptr;

    GeometryBuffer(MeshArrangement arrangement, uint32_t attributes);
    void resize(uint32_t vertex_count);

    MeshArrangement arrangement() const;
    uint32_t attributes() const;
    uint32_t stride() const;
    MaterialID material() const;

    float* vertex(uint32_t index);
    int32_t offset(VertexAttribute attr); ///< Offset into the vertex data that stores the attribute
    uint32_t count() const;

    GLuint vbo();

    float* start() {
        assert(!buffer_.empty());
        return vertex(0);
    }

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
};

}

#endif // GEOMETRY_BUFFER_H
