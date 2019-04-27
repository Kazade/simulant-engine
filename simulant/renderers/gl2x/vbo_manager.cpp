#include "vbo_manager.h"

namespace smlt {

void GPUBuffer::bind_vbos() {
    vertex_vbo->bind(vertex_vbo_slot);
    index_vbo->bind(index_vbo_slot);
}

GPUBuffer VBOManager::update_and_fetch_buffers(Renderable *renderable) {
    const auto& vdata = renderable->vertex_data();
    const auto& idata = renderable->index_data();

    uuid64 vid = vdata->uuid();
    uuid64 iid = idata->uuid();

    auto vit = vertex_data_slots_.find(vid);
    auto iit = index_data_slots_.find(iid);

    VBO* vvbo = nullptr, *ivbo = nullptr;
    VBOSlot vslot = 0, islot = 0;

    bool upload_vdata = false, upload_idata = false;

    if(vit == vertex_data_slots_.end()) {
        /* Could be dedicated? */
        auto dvit = dedicated_vertex_vbos_.find(vdata->uuid());
        if(dvit != dedicated_vertex_vbos_.end()) {
            vvbo = dvit->second.get();
            vslot = 0;
        } else {
            /* OK, first time we've seen this vertex data, let's allocate! */
            auto vpair = allocate_slot(vdata);
            vertex_data_slots_.insert(std::make_pair(vid, vpair));

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
        vvbo->release_slot(vslot);
        auto vpair = allocate_slot(vdata);
        vertex_data_slots_[vid] = vpair;
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

    if(iit == index_data_slots_.end()) {
        /* Could be dedicated? */
        auto diit = dedicated_index_vbos_.find(idata->uuid());
        if(diit != dedicated_index_vbos_.end()) {
            ivbo = diit->second.get();
            islot = 0;
        } else {
            /* OK, first time we've seen this index data, let's allocate! */
            auto ipair = allocate_slot(idata);
            index_data_slots_.insert(std::make_pair(iid, ipair));
            upload_idata = true;

            ivbo = ipair.first;
            islot = ipair.second;
        }

    } else {
        /* We've seen this before... does it need updating? */
        ivbo = iit->second.first;
        islot = iit->second.second;
    }

    if(idata->data_size() > ivbo->slot_size_in_bytes()) {
        /* Data size increased past the slot size, we need to free and reallocate */
        ivbo->release_slot(islot);
        auto ipair = allocate_slot(idata);
        index_data_slots_[iid] = ipair;
        ivbo = ipair.first;
        islot = ipair.second;
        upload_idata = true;
    }

    if(idata->last_updated() > ivbo->slot_last_updated(islot)) {
        upload_idata = true;
    }

    if(upload_idata) {
        /* Index data changed since previous upload, so upload again */
        ivbo->bind(islot);
        ivbo->upload(islot, idata);
    }

    GPUBuffer buffer;

    assert(vvbo->target() == GL_ARRAY_BUFFER);
    assert(ivbo->target() == GL_ELEMENT_ARRAY_BUFFER);

    buffer.index_vbo = ivbo;
    buffer.vertex_vbo = vvbo;
    buffer.index_vbo_slot = islot;
    buffer.vertex_vbo_slot = vslot;

    return buffer;
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
        return std::make_pair(pair.second.get(), 0);
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

        VBO* vbo = it->second.get();
        return std::make_pair(
            vbo, vbo->allocate_slot()
        );
    }
}

std::pair<VBO *, VBOSlot> VBOManager::allocate_slot(const IndexData *index_data) {
    auto required_size = index_data->data_size();
    auto index_type = index_data->index_type();

    if(required_size >= int(VBO_SLOT_SIZE_512K)) {
        /* Use a dedicated VBO */
        auto pair = std::make_pair(index_data->uuid(), DedicatedVBO::create(required_size, index_type));
        dedicated_index_vbos_.insert(pair);
        return std::make_pair(pair.second.get(), 0);
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

        VBO* vbo = it->second.get();
        return std::make_pair(
            vbo, vbo->allocate_slot()
        );
    }
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
