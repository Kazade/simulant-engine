#include "vbo_manager.h"

namespace smlt {

void GPUBuffer::sync_data_from_renderable(smlt::Renderable *renderable) {
    vertex_vbo->upload(vertex_vbo_slot, renderable->vertex_data());
    index_vbo->upload(index_vbo_slot, renderable->index_data());

    last_updated = TimeKeeper::now_in_us();
}

void GPUBuffer::bind_vbos() {
    vertex_vbo->bind(vertex_vbo_slot);
    index_vbo->bind(index_vbo_slot);
}

GPUBuffer *VBOManager::find_buffer(Renderable *renderable) {
    auto id = renderable->uuid();
    auto it = gpu_buffers_.find(id);
    if(it == gpu_buffers_.end()) {
        return nullptr;
    }

    GPUBuffer* buffers = &it->second;

    if((renderable->vertex_data()->data_size() > (int) buffers->vertex_vbo->slot_size()) ||
        (renderable->index_data()->data_size() > (int) buffers->index_vbo->slot_size())) {

        L_DEBUG_VBO("Renderable exceeds slot size, reallocating");

        buffers = reallocate_buffer(renderable);
    }

    return buffers;
}

GPUBuffer* VBOManager::reallocate_buffer(Renderable* renderable) {
    GPUBuffer* buffers = &gpu_buffers_.at(renderable->uuid());

    buffers->vertex_vbo->release_slot(buffers->vertex_vbo_slot);
    buffers->index_vbo->release_slot(buffers->index_vbo_slot);

    const auto& vdata = renderable->vertex_data();
    auto vpair = allocate_slot(
        vdata->specification(),
        vdata->data_size()
    );

    buffers->vertex_vbo = vpair.first;
    buffers->vertex_vbo_slot = vpair.second;

    const auto& idata = renderable->index_data();
    auto ipair = allocate_slot(
        idata->index_type(),
        idata->data_size()
    );

    buffers->index_vbo = ipair.first;
    buffers->index_vbo_slot = ipair.second;
    buffers->last_updated = 0;

    return buffers;
}

GPUBuffer *VBOManager::allocate_buffer(Renderable *renderable) {
    auto existing = find_buffer(renderable);
    if(existing) {
        return existing;
    }

    auto id = renderable->uuid();

    GPUBuffer buffer;

    const auto& vdata = renderable->vertex_data();
    auto vpair = allocate_slot(
        vdata->specification(),
        vdata->data_size()
    );

    buffer.vertex_vbo = vpair.first;
    buffer.vertex_vbo_slot = vpair.second;

    const auto& idata = renderable->index_data();
    auto ipair = allocate_slot(
        idata->index_type(),
        idata->data_size()
    );

    buffer.index_vbo = ipair.first;
    buffer.index_vbo_slot = ipair.second;
    buffer.last_updated = 0;

    gpu_buffers_.insert(std::make_pair(id, buffer));
    return &gpu_buffers_[id];
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

std::pair<VBO *, VBOSlot> VBOManager::allocate_slot(VertexSpecification spec, uint32_t required_size) {
    auto size = calc_vbo_slot_size(required_size);

    auto& entry = shared_vertex_vbos_[int(size) / 1024];
    auto it = entry.find(spec);
    if(it == entry.end()) {
        // Create new VBO
        auto new_vbo = VBO::create(size, spec);
        entry.insert(std::make_pair(spec, new_vbo));
        it = entry.find(spec);
    }

    VBO* vbo = it->second.get();
    return std::make_pair(
        vbo, vbo->allocate_slot()
    );
}

std::pair<VBO *, VBOSlot> VBOManager::allocate_slot(IndexType type, uint32_t required_size) {
    auto size = calc_vbo_slot_size(required_size);

    auto& entry = shared_index_vbos_[int(size) / 1024];
    auto it = entry.find(type);
    if(it == entry.end()) {
        // Create new VBO
        auto new_vbo = VBO::create(size, type);
        entry.insert(std::make_pair(type, new_vbo));
        it = entry.find(type);
    }

    VBO* vbo = it->second.get();
    return std::make_pair(
        vbo, vbo->allocate_slot()
    );
}

VBOSlot VBO::allocate_slot() {
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

void VBO::upload(VBOSlot slot, const VertexData *vertex_data) {
    const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
    uint32_t offset = (slot % SLOTS_PER_BUFFER) * slot_size_in_bytes_;

    bind(slot);
    GLCheck(glBufferSubData, type_, offset, vertex_data->data_size(), vertex_data->data());
}

void VBO::upload(VBOSlot slot, const IndexData *index_data) {
    const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
    uint32_t offset = (slot % SLOTS_PER_BUFFER) * slot_size_in_bytes_;

    bind(slot);
    GLCheck(glBufferSubData, type_, offset, index_data->data_size(), index_data->data());
}

void VBO::bind(VBOSlot slot) {
    const int SLOTS_PER_BUFFER = (VBO_SIZE / slot_size_in_bytes_);
    GLuint vbo_id = gl_ids_[slot / SLOTS_PER_BUFFER];
    GLCheck(glBindBuffer, type_, vbo_id);
}

void VBO::allocate_new_gl_buffer() {
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
