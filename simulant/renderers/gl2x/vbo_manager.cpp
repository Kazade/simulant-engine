#include "vbo_manager.h"

#include "../../utils/skinning.h"

namespace smlt {

namespace {
/* Drop a skinned slot's cache entry if it hasn't been asked for in this
 * long - handles an Armature (or its bound mesh) going away or simply no
 * longer being drawn, without needing a destruction signal from something
 * as short-lived as a SkinningInfo. */
const uint64_t SKINNED_SLOT_IDLE_TIMEOUT_US = 2 * 1000 * 1000;
const uint32_t SKINNED_SLOT_SWEEP_INTERVAL = 256;
} // namespace

void GPUBuffer::bind_vbos() {
    vertex_vbo->bind(vertex_vbo_slot);
    if(index_vbo) {
        index_vbo->bind(index_vbo_slot);
    }
}

void VBOManager::on_index_data_destroyed(IndexData* index_data) {
    release_slot(index_data);
}

void VBOManager::on_vertex_data_destroyed(VertexData* vertex_data) {
    release_slot(vertex_data);
}

template<typename Data>
std::pair<VBO*, VBOSlot> VBOManager::perform_fetch_or_upload(const Data* vdata, VBOManager::DedicatedMap& dedicated_vbos, VBOManager::SlotMap& data_slots) {
    uuid64 vid = vdata->uuid();

    auto vit = data_slots.find(vid);

    VBO* vvbo = nullptr;
    VBOSlot vslot = 0;

    bool upload_vdata = false;

    if(vit == data_slots.end()) {
        /* First time we've seen this data — allocate a slot.
         * allocate_slot always inserts into both data_slots and (if dedicated)
         * dedicated_vbos, so there is no need to check dedicated_vbos
         * separately here. */
        auto vpair = allocate_slot(vdata);
        vvbo = vpair.first;
        vslot = vpair.second;
        upload_vdata = true;
    } else {
        vvbo = vit->second.first;
        vslot = vit->second.second;
    }

    if(vdata->data_size() > vvbo->slot_size_in_bytes()) {
        /* Data size grew past the slot size — free and reallocate. */
        release_slot(vdata);
        auto vpair = allocate_slot(vdata);
        vvbo = vpair.first;
        vslot = vpair.second;
        upload_vdata = true;
    }

    if(vdata->last_updated() > vvbo->slot_last_updated(vslot)) {
        upload_vdata = true;
    }

    assert(vvbo);

    if(upload_vdata) {
        /* upload() binds the VBO internally before writing. */
        vvbo->upload(vslot, vdata);
    }

    return std::make_pair(vvbo, vslot);
}

std::pair<VBO*, VBOSlot>
VBOManager::fetch_skinned_vertex_slot(const Renderable* renderable) {
    auto& entry = skinned_slots_[renderable->skinning];

    entry.last_seen_us = TimeKeeper::now_in_us();

    const VertexData* source = renderable->vertex_data;

    if(!entry.persistent_output) {
        entry.persistent_output =
            std::make_shared<VertexData>(source->vertex_specification());
    }

    /* Only actually re-skin (and, via VertexData::done(), trigger a
     * re-upload below) when the pose has genuinely changed since we last
     * saw it - mirrors the dirty-check perform_fetch_or_upload already does
     * for ordinary vertex data. */
    if(entry.last_generation_skinned != renderable->skinning->generation) {
        if(entry.persistent_output->vertex_specification() !=
           source->vertex_specification()) {
            entry.persistent_output->reset(source->vertex_specification());
        }

        source->clone_into(*entry.persistent_output);
        skin_vertices(source, renderable->skinning->joint_matrices,
                      renderable->skinning->joint_count,
                      entry.persistent_output.get());

        entry.last_generation_skinned = renderable->skinning->generation;
    }

    if(++skinned_slot_fetch_count_ % SKINNED_SLOT_SWEEP_INTERVAL == 0) {
        evict_stale_skinned_slots();
    }

    return perform_fetch_or_upload(entry.persistent_output.get(),
                                   dedicated_vertex_vbos_, vertex_data_slots_);
}

void VBOManager::evict_stale_skinned_slots() {
    const uint64_t now = TimeKeeper::now_in_us();

    for(auto it = skinned_slots_.begin(); it != skinned_slots_.end();) {
        if(now - it->second.last_seen_us > SKINNED_SLOT_IDLE_TIMEOUT_US) {
            /* Dropping persistent_output fires its destruction signal,
             * which release_slot() is already connected to. */
            it = skinned_slots_.erase(it);
        } else {
            ++it;
        }
    }
}

GPUBuffer VBOManager::update_and_fetch_buffers(const Renderable *renderable) {
    auto vpair = (renderable->skinning)
                     ? fetch_skinned_vertex_slot(renderable)
                     : perform_fetch_or_upload(renderable->vertex_data,
                                               dedicated_vertex_vbos_,
                                               vertex_data_slots_);

    assert(vpair.first->target() == GL_ARRAY_BUFFER);

    GPUBuffer buffer;
    buffer.vertex_vbo = vpair.first;
    buffer.vertex_vbo_slot = vpair.second;

    if(renderable->index_data) {
        const auto& idata = renderable->index_data;
        auto ipair = perform_fetch_or_upload(idata, dedicated_index_vbos_, index_data_slots_);
        assert(ipair.first->target() == GL_ELEMENT_ARRAY_BUFFER);

        buffer.index_vbo = ipair.first;
        buffer.index_vbo_slot = ipair.second;
    }

    return buffer;
}

uint32_t VBOManager::dedicated_buffer_count() const {
    return dedicated_index_vbos_.size() + dedicated_vertex_vbos_.size();
}

VBOManager::~VBOManager() {
    for(auto& pair: vdata_destruction_connections_) {
        pair.second.disconnect();
    }

    for(auto& pair: idata_destruction_connections_) {
        pair.second.disconnect();
    }
}

VBOSlotSize VBOManager::calc_vbo_slot_size(uint32_t required_size_in_bytes) {
    auto next_pow2 = [](uint32_t x) -> uint32_t {
        uint32_t power = 1;
        while(power < x)
            power *= 2;
        return power;
    };

    VBOSlotSize size = VBO_SLOT_SIZE_1K;

    auto check = next_pow2(required_size_in_bytes);

    assert(check <= (int) smlt::VBO_SLOT_SIZE_512K);

    if(check <= (int) VBO_SLOT_SIZE_1K) {
        size = VBO_SLOT_SIZE_1K;
    } else {
        size = (VBOSlotSize) check;
    }

    return size;
}

std::pair<VBO *, VBOSlot> VBOManager::allocate_slot(const VertexData *vertex_data) {
    auto required_size = vertex_data->data_size();
    auto spec = vertex_data->vertex_specification();

    if(required_size >= int(VBO_SLOT_SIZE_512K)) {
        /* Use a dedicated VBO */
        auto pair = std::make_pair(vertex_data->uuid(), DedicatedVBO::create(required_size, spec));
        dedicated_vertex_vbos_.insert(pair);

        connect_destruction_signal(vertex_data);

        auto vpair = std::make_pair(pair.second.get(), pair.second->allocate_slot());
        vertex_data_slots_.insert(std::make_pair(vertex_data->uuid(), vpair));
        return vpair;
    } else {
        /* Use one of the shared VBOs */
        auto size = calc_vbo_slot_size(required_size);

        auto idx = log2(int(size)) - 10; // Convert enum value to index
        auto& entry = shared_vertex_vbos_[idx];
        auto it = entry.find(spec);
        if(it == entry.end()) {
            it = entry.insert(std::make_pair(spec, SharedVBO::create(size, spec))).first;
        }

        connect_destruction_signal(vertex_data);

        VBO* vbo = it->second.get();
        auto vpair = std::make_pair(vbo, vbo->allocate_slot());
        vertex_data_slots_.insert(std::make_pair(vertex_data->uuid(), vpair));
        return vpair;
    }
}

void VBOManager::release_slot(const VertexData *vertex_data) {
    auto vid = vertex_data->uuid();
    auto it = vertex_data_slots_.find(vid);
    if(it != vertex_data_slots_.end()) {
        it->second.first->release_slot(it->second.second);
        vertex_data_slots_.erase(it);
    }

    /* If this was a dedicate VBO, then destroy it */
    auto dit = dedicated_vertex_vbos_.find(vid);
    if(dit != dedicated_vertex_vbos_.end()) {
        dedicated_vertex_vbos_.erase(vid);
    }

    disconnect_destruction_signal(vertex_data);
}

void VBOManager::release_slot(const IndexData *index_data) {
    auto vid = index_data->uuid();
    auto it = index_data_slots_.find(vid);
    if(it != index_data_slots_.end()) {
        it->second.first->release_slot(it->second.second);
        index_data_slots_.erase(it);
    }

    /* If this was a dedicate VBO, then destroy it */
    auto dit = dedicated_index_vbos_.find(vid);
    if(dit != dedicated_index_vbos_.end()) {
        dedicated_index_vbos_.erase(vid);
    }

    disconnect_destruction_signal(index_data);
}

std::pair<VBO *, VBOSlot> VBOManager::allocate_slot(const IndexData *index_data) {
    auto required_size = index_data->data_size();
    auto index_type = index_data->index_type();

    if(required_size >= int(VBO_SLOT_SIZE_512K)) {
        /* Use a dedicated VBO */
        auto pair = std::make_pair(index_data->uuid(), DedicatedVBO::create(required_size, index_type));
        dedicated_index_vbos_.insert(pair);

        connect_destruction_signal(index_data);

        auto ipair = std::make_pair(pair.second.get(), pair.second->allocate_slot());
        index_data_slots_.insert(std::make_pair(index_data->uuid(), ipair));
        return ipair;
    } else {
        /* Use one of the shared VBOs */
        auto size = calc_vbo_slot_size(required_size);

        auto idx = log2(int(size)) - 10; // Convert enum value to index
        auto& entry = shared_index_vbos_[idx];
        auto it = entry.find(index_type);
        if(it == entry.end()) {
            it = entry.insert(std::make_pair(index_type, SharedVBO::create(size, index_type))).first;
        }

        connect_destruction_signal(index_data);

        VBO* vbo = it->second.get();
        auto ipair = std::make_pair(vbo, vbo->allocate_slot());
        index_data_slots_.insert(std::make_pair(index_data->uuid(), ipair));
        return ipair;
    }
}

void VBOManager::disconnect_destruction_signal(const VertexData *vertex_data) {
    auto it = vdata_destruction_connections_.find(vertex_data->uuid());
    if(it != vdata_destruction_connections_.end()) {
        it->second.disconnect();
        vdata_destruction_connections_.erase(it);
    }
}

void VBOManager::disconnect_destruction_signal(const IndexData *index_data) {
    auto it = idata_destruction_connections_.find(index_data->uuid());
    if(it != idata_destruction_connections_.end()) {
        it->second.disconnect();
        idata_destruction_connections_.erase(it);
    }
}

void VBOManager::connect_destruction_signal(const VertexData* vdata) {
    auto uuid = vdata->uuid();
    auto existing = vdata_destruction_connections_.find(uuid);
    if(existing != vdata_destruction_connections_.end()) {
        existing->second.disconnect();
        vdata_destruction_connections_.erase(existing);
    }

    vdata_destruction_connections_.insert(std::make_pair(
        uuid,
        vdata->signal_destruction().connect(
            std::bind(&VBOManager::on_vertex_data_destroyed, this, std::placeholders::_1)
        )
    ));
}

void VBOManager::connect_destruction_signal(const IndexData* vdata) {
    auto uuid = vdata->uuid();
    auto existing = idata_destruction_connections_.find(uuid);
    if(existing != idata_destruction_connections_.end()) {
        existing->second.disconnect();
        idata_destruction_connections_.erase(existing);
    }

    idata_destruction_connections_.insert(std::make_pair(
        uuid,
        vdata->signal_destruction().connect(
            std::bind(&VBOManager::on_index_data_destroyed, this, std::placeholders::_1)
        )
    ));
}

VBOSlot SharedVBO::allocate_slot() {
    if(!free_slots_.empty()) {
        VBOSlot slot = free_slots_.front();
        free_slots_.pop();

        L_DEBUG_VBO(_F("Grabbed existing free slot {0}").format(slot));
        return slot;
    } else {
        // Allocate a new GL buffer
        // which creates new free slots
        allocate_new_gl_buffer();

        // Recurse
        return allocate_slot();
    }
}

void SharedVBO::release_slot(VBOSlot slot) {
    L_DEBUG_VBO(_F("Releasing slot {0}").format(slot));
    free_slots_.push(slot);
}

void SharedVBO::upload(VBOSlot slot, const VertexData *vertex_data) {
    const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
    uint32_t offset = (slot % SLOTS_PER_BUFFER) * slot_size_in_bytes_;

    bind(slot);
    GLCheck(glBufferSubData, type_, offset, vertex_data->data_size(), vertex_data->data());

    metas_[slot].last_updated = TimeKeeper::now_in_us();
}

void SharedVBO::upload(VBOSlot slot, const IndexData *index_data) {
    const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
    uint32_t offset = (slot % SLOTS_PER_BUFFER) * slot_size_in_bytes_;

    bind(slot);

    GLCheck(glBufferSubData, type_, offset, index_data->data_size(), index_data->data());

    metas_[slot].last_updated = TimeKeeper::now_in_us();
}

void SharedVBO::bind(VBOSlot slot) {
    const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
    GLuint vbo_id = gl_ids_[slot / SLOTS_PER_BUFFER];

    GLCheck(glBindBuffer, type_, vbo_id);
}

void DedicatedVBO::upload(VBOSlot, const VertexData* vertex_data) {
    bind(0);
    GLCheck(glBufferData, type_, vertex_data->data_size(), vertex_data->data(), GL_DYNAMIC_DRAW);
    last_updated_ = TimeKeeper::now_in_us();
}

void DedicatedVBO::upload(VBOSlot, const IndexData* index_data) {
    bind(0);
    GLCheck(glBufferData, type_, index_data->data_size(), index_data->data(), GL_DYNAMIC_DRAW);
    last_updated_ = TimeKeeper::now_in_us();
}

void DedicatedVBO::bind(VBOSlot) {
    if(!gl_id_) {
        GLCheck(glGenBuffers, 1, &gl_id_);
    }

    GLCheck(glBindBuffer, type_, gl_id_);
}

VBOSlot DedicatedVBO::allocate_slot() {
    assert(!allocated_);
    allocated_ = true;
    return 0;
}

void DedicatedVBO::release_slot(VBOSlot slot) {
    assert(slot == 0);
    assert(allocated_);
    allocated_ = false;

    if(gl_id_) {
        GLCheck(glDeleteBuffers, 1, &gl_id_);
        gl_id_ = 0;
    }
}

void SharedVBO::allocate_new_gl_buffer() {
    /* Create a new GL VBO, then push back the
         * free slot IDS */

    L_DEBUG_VBO(_F("Allocating new GL buffer for target {0}").format(type_));

    GLuint buffer;

    GLCheck(glGenBuffers, 1, &buffer);
    GLCheck(glBindBuffer, type_, buffer);

    /* Allocate GPU storage without uploading CPU data. glBufferSubData is
     * always used for individual slot writes, so the initial content is never
     * read before being overwritten. */
    GLCheck(glBufferData, type_, VBO_SIZE, nullptr, GL_DYNAMIC_DRAW);

    const auto slots_per_buffer = VBO_SIZE / slot_size_;

    /* Push new meta data for each slot */
    metas_.resize((gl_ids_.size() + 1) * slots_per_buffer);

    /* Push new free slots */
    auto offset = gl_ids_.size() * slots_per_buffer;
    for(VBOSlot i = 0; i < slots_per_buffer; ++i) {
        free_slots_.push(offset + i);
    }

    /* Store the new VBO ID */
    gl_ids_.push_back(buffer);
}

}
