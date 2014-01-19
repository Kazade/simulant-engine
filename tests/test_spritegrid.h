#ifndef TEST_SPRITEGRID_H
#define TEST_SPRITEGRID_H

#include <kaztest/kaztest.h>

#include "../kglt/extra.h"
#include "global.h"

using namespace kglt;

class SpriteGridTests : public TestCase {
private:
    extra::SpriteGrid::ptr grid_;

public:
    void set_up() {
        if(!window) {
            window = Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        grid_ = extra::SpriteGrid::create(window->scene(), StageID(), 64, 32);
    }

    void test_chunk() {
        auto chunk = grid_->chunk(0, 0);
        assert_equal(grid_->chunk_at(0), chunk); //Should return the first chunk

        chunk = grid_->chunk(17, 0);
        assert_equal(grid_->chunk_at(1), chunk); //Should return the second chunk

        chunk = grid_->chunk(0, 17);
        assert_equal(grid_->chunk_at(4), chunk); //Should return the first chunk on the second row
    }

    void test_chunk_tile_index() {
        auto result = grid_->chunk_tile_index(3, 0);

        assert_equal(grid_->chunk_at(0), result.first);
        assert_equal(3, result.second);

        result = grid_->chunk_tile_index(0, 16);

        assert_equal(grid_->chunk_at(4), result.first);
        assert_equal(0, result.second);
    }
};

#endif // TEST_SPRITEGRID_H
