#pragma once

#include <cstdint>
#include <array>
#include <unordered_map>
#include <queue>

#include "../../meshes/mesh.h"
#include "../../generic/managed.h"
#include "../../time_keeper.h"
#include "../glad/glad/glad.h"
#include "../../utils/gl_error.h"
#include "../batching/renderable.h"

#include "../../deps/kazlog/kazlog.h"

#define __LOGGER__ "/smlt/vbo"

#define L_DEBUG_VBO(txt) kazlog::get_logger(__LOGGER__)->debug(txt, __FILE__, __LINE__)
#define L_INFO_VBO(txt) kazlog::get_logger(__LOGGER__)->info(txt, __FILE__, __LINE__)
#define L_WARN_VBO(txt) kazlog::get_logger(__LOGGER__)->warn(txt, __FILE__, __LINE__)
#define L_ERROR_VBO(txt) kazlog::get_logger(__LOGGER__)->error(txt, __FILE__, __LINE__)

namespace smlt {

/* The size of VBO to allocate */
const uint32_t VBO_SIZE = 1024 * 512;

typedef uint32_t VBOSlot;

class VBO;

struct GPUBuffer {
    /* Slot into the VBO */
    VBO* vertex_vbo;
    VBO* index_vbo;

    VBOSlot vertex_vbo_slot;
    VBOSlot index_vbo_slot;

    uint64_t last_updated;

    void sync_data_from_renderable(Renderable* renderable);
    void bind_vbos();
};

enum VBOSlotSize {
    VBO_SLOT_SIZE_1K = (1 << 10),
    VBO_SLOT_SIZE_2K = (1 << 11),
    VBO_SLOT_SIZE_4K = (1 << 12),
    VBO_SLOT_SIZE_8K = (1 << 13),
    VBO_SLOT_SIZE_16K = (1 << 14),
    VBO_SLOT_SIZE_32K = (1 << 15),
    VBO_SLOT_SIZE_64K = (1 << 16),
    VBO_SLOT_SIZE_128K = (1 << 17),
    VBO_SLOT_SIZE_256K = (1 << 18),
    VBO_SLOT_SIZE_512K = (1 << 19)
};

const int VBO_SLOT_SIZE_COUNT = 11;

class VBO:
    public Managed<VBO> {

public:
    VBO(VBOSlotSize slot_size, VertexSpecification spec):
        slot_size_(slot_size),
        slot_size_in_bytes_(int(slot_size)),
        spec_(spec),
        type_(GL_ARRAY_BUFFER) {}

    VBO(VBOSlotSize slot_size, IndexType type):
        slot_size_(slot_size),
        slot_size_in_bytes_(int(slot_size)),
        index_type_(type),
        type_(GL_ELEMENT_ARRAY_BUFFER) {}

    uint64_t slot_last_updated(VBOSlot slot) {
        return metas_[slot].last_updated;
    }

    VBOSlot allocate_slot();

    void release_slot(VBOSlot slot) {
        L_DEBUG_VBO(_F("Releasing slot {0}").format(slot));
        free_slots_.push(slot);
    }

    void upload(VBOSlot slot, const VertexData* vertex_data);

    void upload(VBOSlot slot, const IndexData* index_data);

    void bind(VBOSlot slot);

    VBOSlotSize slot_size() const { return slot_size_; }

    uint32_t byte_offset(VBOSlot slot) {
        const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
        return (slot % SLOTS_PER_BUFFER) * slot_size_in_bytes_;
    }

private:
    void allocate_new_gl_buffer();

    VBOSlotSize slot_size_;
    uint32_t slot_size_in_bytes_;

    VertexSpecification spec_;
    IndexType index_type_;
    GLenum type_;
    std::queue<VBOSlot> free_slots_;
    std::vector<GLuint> gl_ids_;

    struct SlotMeta {
        uint64_t last_updated = 0;
        uint32_t uploaded_size = 0;
    };

    std::vector<SlotMeta> metas_;
};

class VBOManager : public Managed<VBOManager> {
public:
    GPUBuffer* find_buffer(Renderable* renderable);
    GPUBuffer* allocate_buffer(Renderable* renderable);
    GPUBuffer* reallocate_buffer(Renderable* renderable);

private:
    VBOSlotSize calc_vbo_slot_size(uint32_t required_size_in_bytes);

    std::pair<VBO*, VBOSlot> allocate_slot(VertexSpecification spec, uint32_t required_size);
    std::pair<VBO*, VBOSlot> allocate_slot(IndexType type, uint32_t required_size);

    /* Buffers for renderables which aren't dynamic and are less than 512k */
    std::array<
        std::unordered_map<VertexSpecification, VBO::ptr>,
        VBO_SLOT_SIZE_COUNT
    > shared_vertex_vbos_;

    std::array<
        std::unordered_map<IndexType, VBO::ptr>,
        VBO_SLOT_SIZE_COUNT
    > shared_index_vbos_;

    std::unordered_map<
        uint64_t,
        GPUBuffer
    > gpu_buffers_;
};

}
