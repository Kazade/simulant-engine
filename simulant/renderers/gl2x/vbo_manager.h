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

#include "../../logging.h"

#define __LOGGER__ "/smlt/vbo"

#define L_DEBUG_VBO(txt) smlt::get_logger(__LOGGER__)->debug(txt, __FILE__, __LINE__)
#define L_INFO_VBO(txt) smlt::get_logger(__LOGGER__)->info(txt, __FILE__, __LINE__)
#define L_WARN_VBO(txt) smlt::get_logger(__LOGGER__)->warn(txt, __FILE__, __LINE__)
#define L_ERROR_VBO(txt) smlt::get_logger(__LOGGER__)->error(txt, __FILE__, __LINE__)

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

class VBO {
public:
    virtual ~VBO() {}

    virtual GLenum target() const = 0;
    virtual void upload(VBOSlot, const VertexData* vertex_data) = 0;
    virtual void upload(VBOSlot, const IndexData* index_data) = 0;
    virtual void bind(VBOSlot) = 0;
    virtual uint64_t slot_last_updated(VBOSlot slot) = 0;
    virtual uint32_t byte_offset(VBOSlot slot) = 0;
    virtual uint32_t slot_size_in_bytes() const = 0;

    virtual VBOSlot allocate_slot() = 0;
    virtual void release_slot(VBOSlot slot) = 0;

    virtual uint32_t used_slot_count() const = 0;
    virtual uint32_t free_slot_count() const = 0;
};

class DedicatedVBO:
    public RefCounted<DedicatedVBO>,
    public VBO {

public:
    DedicatedVBO(uint32_t size, VertexSpecification spec):
        size_in_bytes_(size),
        spec_(spec),
        type_(GL_ARRAY_BUFFER) {}

    DedicatedVBO(uint32_t size, IndexType index_type):
        size_in_bytes_(size),
        index_type_(index_type),
        type_(GL_ELEMENT_ARRAY_BUFFER) {}

    ~DedicatedVBO() {
        try {
            if(gl_id_) glDeleteBuffers(1, &gl_id_);
        } catch(...) {
            L_WARN("Exception while deleting GL VBO");
        }
    }

    uint64_t slot_last_updated(VBOSlot slot) { assert(slot == 0); return last_updated_; }
    GLenum target() const { return type_; }

    void upload(VBOSlot, const VertexData* vertex_data);
    void upload(VBOSlot, const IndexData* index_data);
    void bind(VBOSlot);

    uint32_t byte_offset(VBOSlot slot) {
        assert(slot == 0);
        return 0;
    }

    uint32_t slot_size_in_bytes() const {
        return size_in_bytes_;
    }

    VBOSlot allocate_slot();
    void release_slot(VBOSlot slot);

    uint32_t used_slot_count() const {
        return (allocated_) ? 1 : 0;
    }

    uint32_t free_slot_count() const {
        return (allocated_) ? 0 : 1;
    }
private:
    uint32_t size_in_bytes_;
    VertexSpecification spec_;
    IndexType index_type_;
    GLenum type_;

    uint64_t last_updated_;
    bool allocated_ = false;
    GLuint gl_id_ = 0;
};

class SharedVBO:
    public RefCounted<SharedVBO>,
    public VBO {

public:
    SharedVBO(VBOSlotSize slot_size, VertexSpecification spec):
        slot_size_(slot_size),
        slot_size_in_bytes_(int(slot_size)),
        spec_(spec),
        type_(GL_ARRAY_BUFFER) {}

    SharedVBO(VBOSlotSize slot_size, IndexType type):
        slot_size_(slot_size),
        slot_size_in_bytes_(int(slot_size)),
        index_type_(type),
        type_(GL_ELEMENT_ARRAY_BUFFER) {}

    uint64_t slot_last_updated(VBOSlot slot) {
        return metas_[slot].last_updated;
    }

    GLenum target() const { return type_; }

    VBOSlot allocate_slot();

    void release_slot(VBOSlot slot);

    void upload(VBOSlot slot, const VertexData* vertex_data);
    void upload(VBOSlot slot, const IndexData* index_data);
    void bind(VBOSlot slot);

    VBOSlotSize slot_size() const { return slot_size_; }
    uint32_t slot_size_in_bytes() const { return int(slot_size_); }

    uint32_t byte_offset(VBOSlot slot) {
        const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
        return (slot % SLOTS_PER_BUFFER) * slot_size_in_bytes_;
    }

    uint32_t used_slot_count() const {
        return metas_.size() - free_slots_.size();
    }

    uint32_t free_slot_count() const {
        return free_slots_.size();
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

class VBOManager : public RefCounted<VBOManager> {
public:
    ~VBOManager();
    GPUBuffer update_and_fetch_buffers(Renderable* renderable);

    uint32_t dedicated_buffer_count() const;

private:
    VBOSlotSize calc_vbo_slot_size(uint32_t required_size_in_bytes);

    std::pair<VBO*, VBOSlot> allocate_slot(const VertexData* vertex_data);
    std::pair<VBO*, VBOSlot> allocate_slot(const IndexData* index_data);

    void release_slot(const VertexData* vertex_data);
    void release_slot(const IndexData* index_data);

    /* Buffers for renderables which aren't dynamic and are less than 512k */
    std::array<
        std::unordered_map<VertexSpecification, SharedVBO::ptr>,
        VBO_SLOT_SIZE_COUNT
    > shared_vertex_vbos_;

    std::array<
        std::unordered_map<IndexType, SharedVBO::ptr>,
        VBO_SLOT_SIZE_COUNT
    > shared_index_vbos_;

    typedef std::unordered_map<uuid64, DedicatedVBO::ptr> DedicatedMap;
    typedef std::unordered_map<uuid64, std::pair<VBO*, VBOSlot>> SlotMap;

    DedicatedMap dedicated_vertex_vbos_;
    DedicatedMap dedicated_index_vbos_;

    SlotMap vertex_data_slots_;
    SlotMap index_data_slots_;

    std::unordered_map<uuid64, sig::connection> vdata_destruction_connections_;
    std::unordered_map<uuid64, sig::connection> idata_destruction_connections_;

    void connect_destruction_signal(const VertexData *vertex_data);
    void connect_destruction_signal(const IndexData *vertex_data);
    void disconnect_destruction_signal(const VertexData *vertex_data);
    void disconnect_destruction_signal(const IndexData *vertex_data);

    void on_vertex_data_destroyed(VertexData* vertex_data);
    void on_index_data_destroyed(IndexData* vertex_data);

    template<typename Data>
    std::pair<VBO*, VBOSlot> perform_fetch_or_upload(const Data*, VBOManager::DedicatedMap&, VBOManager::SlotMap&);

};

}
