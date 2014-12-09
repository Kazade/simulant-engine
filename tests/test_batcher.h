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
        assert_false(root_group_->exists<TextureGroup>(TextureGroupData(0, TextureID())));

        root_group_->get_or_create<TextureGroup>(TextureGroupData(0, TextureID())).get_or_create<TextureGroup>(TextureGroupData(1, TextureID(1)));

        assert_true(root_group_->exists<TextureGroup>(TextureGroupData(0, TextureID())));
        assert_true(root_group_->get<TextureGroup>(TextureGroupData(0, TextureID())).exists<TextureGroup>(TextureGroupData(1, TextureID(1))));
        assert_false(root_group_->get<TextureGroup>(TextureGroupData(0, TextureID())).exists<TextureGroup>(TextureGroupData(1, TextureID(2))));
    }    

private:
    kglt::RootGroup::ptr root_group_;
};

}

#endif // TEST_BATCHER_H
