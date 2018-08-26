#pragma once

#include "simulant/simulant.h"
#include "../global.h"
#include "../../simulant/hardware_buffer.h"
#include "../../simulant/renderers/gl2x/buffer_manager.h"

namespace {

using namespace smlt;

class GL2Tests:
    public SimulantTestCase {

private:
    GL2BufferManager buffer_manager_;

public:
    GL2Tests():
        buffer_manager_(nullptr) {

    }

    void test_that_buffers_can_be_allocated() {
        auto buffer = buffer_manager_.allocate(10, smlt::HARDWARE_BUFFER_VERTEX_ATTRIBUTES, SHADOW_BUFFER_DISABLED);
        assert_equal(10u, buffer->size());
    }

    void test_that_buffers_can_be_released() {
        auto buffer = buffer_manager_.allocate(10, smlt::HARDWARE_BUFFER_VERTEX_ATTRIBUTES, SHADOW_BUFFER_DISABLED);
        assert_equal(10u, buffer->size());
        buffer->release();
        assert_true(buffer->is_dead());

        buffer->release(); // Second release should be no-op

        const uint8_t random_data[8] = {0};

        auto func = [&]() {
            buffer->upload(random_data, 8);
        };

        // Trying to upload data to a dead buffer should raise a logic error
        assert_raises(std::logic_error, func);
    }

    void test_that_vertex_data_is_uploaded() {
        smlt::VertexSpecification spec(smlt::VERTEX_ATTRIBUTE_3F);
        smlt::VertexData data(spec);

        data.position(smlt::Vec3());
        data.move_next();

        data.position(smlt::Vec3());
        data.done();

        auto small_buffer = buffer_manager_.allocate(spec.stride(), smlt::HARDWARE_BUFFER_VERTEX_ATTRIBUTES, SHADOW_BUFFER_DISABLED);

        // Buffer is too small for the specified data, should raise an error
        assert_raises(std::out_of_range, [&]() {
            small_buffer->upload(data);
        });

        auto buffer = buffer_manager_.allocate(spec.stride() * 2, smlt::HARDWARE_BUFFER_VERTEX_ATTRIBUTES, SHADOW_BUFFER_DISABLED);
        buffer->upload(data); //Should succeed
    }
};

}
