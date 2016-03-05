#ifndef TEST_BATCHER_H
#define TEST_BATCHER_H

#include <kaztest/kaztest.h>

#include "kglt/kglt.h"
#include "global.h"

namespace {

using namespace kglt;

class BatcherTest : public KGLTTestCase {
public:
    void set_up() {
        KGLTTestCase::set_up();

        root_group_.reset(new RootGroup(*window, StageID(), CameraID()));
    }

    void test_group_creation() {
        std::vector<GLuint> units;
        units.push_back(1);

        assert_false(root_group_->exists<TextureGroup>(TextureGroupData(units)));

        root_group_->get_or_create<TextureGroup>(TextureGroupData(units));

        assert_true(root_group_->exists<TextureGroup>(TextureGroupData(units)));
    }    

private:
    kglt::RootGroup::ptr root_group_;
};

}

#endif // TEST_BATCHER_H
