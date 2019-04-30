#include "vbo_manager.h"

namespace smlt {

void GPUBuffer::bind_vbos() {
    vertex_vbo->bind(vertex_vbo_slot);
    index_vbo->bind(index_vbo_slot);
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
        /* Could be dedicated? */
        auto dvit = dedicated_vbos.find(vid);
        if(dvit != dedicated_vbos.end()) {
            vvbo = dvit->second.get();
            vslot = 0;
        } else {
            /* OK, first time we've seen this vertex data, let's allocate! */
            auto vpair = allocate_slot(vdata);
            vvbo = vpair.first;
            vslot = vpair.second;

            upload_vdata = true;
        }
    } else {
        /* We've seen this before... does it need updating? */
        vvbo = vit->second.first;
        vslot = vit->second.second;
    }

    if(vdata->data_size() > vvbo->slot_size_in_bytes()) {
        /* Data size increased past the slot size, we need to free and reallocate */
        /* Data size increased past the slot size, we need to free and reallocate */
        release_slot(vdata);
        auto vpair = allocate_slot(vdata);
        vvbo = vpair.first;
        vslot = vpair.second;
        upload_vdata = true;
    }

    // FIXME: What if the vertex buffer reduces in size to below the next slot, do we
    // bother reallocating?

    if(vdata->last_updated() > vvbo->slot_last_updated(vslot)) {
        upload_vdata = true;
    }

    assert(vvbo);

    if(upload_vdata) {
        /* Vertex data changed since previous upload, so upload again */
        vvbo->bind(vslot);
        vvbo->upload(vslot, vdata);
    }

    return std::make_pair(vvbo, vslot);
}

GPUBuffer VBOManager::update_and_fetch_buffers(Renderable *renderable) {
    const auto& vdata = renderable->vertex_data();
    const auto& idata = renderable->index_data();

    auto vpair = perform_fetch_or_upload(vdata, dedicated_vertex_vbos_, vertex_data_slots_);
    auto ipair = perform_fetch_or_upload(idata, dedicated_index_vbos_, index_data_slots_);

    GPUBuffer buffer;

    assert(vpair.first->target() == GL_ARRAY_BUFFER);
    assert(ipair.first->target() == GL_ELEMENT_ARRAY_BUFFER);

    buffer.index_vbo = ipair.first;
    buffer.vertex_vbo = vpair.first;
    buffer.index_vbo_slot = ipair.second;
    buffer.vertex_vbo_slot = vpair.second;

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

    switch(next_pow2(required_size_in_bytes)) {
    case (1 << 1):
    case (1 << 2):
    case (1 << 3):
    case (1 << 4):
    case (1 << 5):
    case (1 << 6):
    case (1 << 7):
    case (1 << 8):
    case (1 << 9):
        size = VBO_SLOT_SIZE_1K;
        break;
    case (1 << 10):
        size = VBO_SLOT_SIZE_2K;
        break;
    case (1 << 11):
        size = VBO_SLOT_SIZE_4K;
        break;
    case (1 << 12):
        size = VBO_SLOT_SIZE_8K;
        break;
    case (1 << 13):
        size = VBO_SLOT_SIZE_16K;
        break;
    case (1 << 14):
        size = VBO_SLOT_SIZE_32K;
        break;
    case (1 << 15):
        size = VBO_SLOT_SIZE_64K;
        break;
    case (1 << 16):
        size = VBO_SLOT_SIZE_128K;
        break;
    case (1 << 17):
        size = VBO_SLOT_SIZE_256K;
        break;
    case (1 << 18):
        size = VBO_SLOT_SIZE_512K;
        break;
    default:
        assert(0 && "Not Implemented");
        size = VBO_SLOT_SIZE_512K;
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

        auto& entry = shared_vertex_vbos_[int(size) / 1024];
        auto it = entry.find(spec);
        if(it == entry.end()) {
            // Create new VBO
            auto new_vbo = SharedVBO::create(size, spec);
            entry.insert(std::make_pair(spec, new_vbo));
            it = entry.find(spec);
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

        auto& entry = shared_index_vbos_[int(size) / 1024];
        auto it = entry.find(index_type);
        if(it == entry.end()) {
            // Create new VBO
            auto new_vbo = SharedVBO::create(size, index_type);
            entry.insert(std::make_pair(index_type, new_vbo));
            it = entry.find(index_type);
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
    GLCheck(glBufferData, type_, vertex_data->data_size(), vertex_data->data(), GL_STATIC_DRAW);
    last_updated_ = TimeKeeper::now_in_us();
}

void DedicatedVBO::upload(VBOSlot, const IndexData* index_data) {
    bind(0);
    GLCheck(glBufferData, type_, index_data->data_size(), index_data->data(), GL_STATIC_DRAW);
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

    // FIXME? Should we delete the GL buffer here?
}

void SharedVBO::allocate_new_gl_buffer() {
    /* Create a new GL VBO, then push back the
         * free slot IDS */

    L_DEBUG_VBO(_F("Allocating new GL buffer for target {0}").format(type_));

    GLuint buffer;

    GLCheck(glGenBuffers, 1, &buffer);
    GLCheck(glBindBuffer, type_, buffer);

    // Upload VBO_SIZE of zeros so we an use buffersubdata afterwards
    std::vector<uint8_t> init_data(VBO_SIZE, 0);

    /* FIXME: usage needs to change based on, well usage */
    GLCheck(glBufferData, type_, VBO_SIZE, &init_data[0], GL_STATIC_DRAW);

    const auto slots_per_buffer = VBO_SIZE / slot_size_;

    /* Push new meta data for each slot */
    metas_.resize(gl_ids_.size() + 1 * slots_per_buffer);

    /* Push new free slots */
    auto offset = gl_ids_.size() * slots_per_buffer;
    for(VBOSlot i = 0; i < slots_per_buffer; ++i) {
        free_slots_.push(offset + i);
    }

    /* Store the new VBO ID */
    gl_ids_.push_back(buffer);
}

}
