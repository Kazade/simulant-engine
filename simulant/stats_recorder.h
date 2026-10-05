#pragma once

#include <cstdint>

#include "generic/managed.h"
#include "types.h"

namespace smlt {

class StatsRecorder:
    public RefCounted<StatsRecorder> {

public:
    uint32_t geometry_visible() const {
        return geometry_visible_;
    }

    void set_geometry_visible(uint32_t value) {
        geometry_visible_ = value;
    }

    uint32_t subactors_rendered() const { return subactors_renderered_; }
    void set_subactors_rendered(uint32_t value) {
        subactors_renderered_ = value;
    }

    float frame_time() const { return frame_time_; }
    void set_frame_time(float value) {
        frame_time_ = value;
    }

    uint32_t frames_per_second() const { return frames_per_second_; }
    void set_frames_per_second(uint32_t value) {
        frames_per_second_ = value;
    }

    uint64_t fixed_steps_run() const { return fixed_steps_run_; }
    uint64_t frames_run() const { return frames_run_; }

    void increment_fixed_steps() { fixed_steps_run_++; }
    void increment_frames() { frames_run_++; }

    void reset_polygons_rendered() {
        polygons_rendered_ = 0;
        vertices_rendered_ = 0;
    }

    /* The number of polygons `element_count` vertices (or indices) make up
     * in the given arrangement */
    static uint32_t polygon_count(MeshArrangement arrangement, uint32_t element_count) {
        switch(arrangement) {
            case MESH_ARRANGEMENT_TRIANGLES: return element_count / 3;
            case MESH_ARRANGEMENT_TRIANGLE_STRIP: return element_count - 2;
            case MESH_ARRANGEMENT_TRIANGLE_FAN: return element_count - 2;
            case MESH_ARRANGEMENT_LINES: return element_count / 2;
            case MESH_ARRANGEMENT_LINE_STRIP: return element_count - 1;
            case MESH_ARRANGEMENT_QUADS: return element_count / 4;
        }
        return 0;
    }

    void increment_polygons_rendered(MeshArrangement arrangement, uint32_t element_count);

    /* For renderers that total up polygon_count() themselves over a pass and
     * report it once, rather than calling increment_polygons_rendered() for
     * every renderable. */
    void add_polygons_rendered(uint32_t count) {
        polygons_rendered_ += count;
    }
    uint32_t polygons_rendered() const {
        return polygons_rendered_;
    }

    /* Vertices submitted for drawing this frame: one per index for indexed
     * geometry. Counted alongside the polygons (increment_polygons_rendered
     * counts both). */
    void add_vertices_rendered(uint32_t count) {
        vertices_rendered_ += count;
    }
    uint32_t vertices_rendered() const {
        return vertices_rendered_;
    }

private:
    float frame_time_ = 0;
    uint32_t subactors_renderered_ = 0;
    uint32_t frames_per_second_ = 0;
    uint32_t geometry_visible_ = 0;

    uint64_t fixed_steps_run_ = 0;
    uint64_t frames_run_ = 0;

    uint32_t polygons_rendered_ = 0;
    uint32_t vertices_rendered_ = 0;
};


}
