#ifndef TEST_BATCHER_H
#define TEST_BATCHER_H

#include "kglt/kazbase/testing.h"

#include "kglt/kglt.h"
#include "global.h"

class BatcherTest : public TestCase {
public:
    void set_up() {
        root_group_.reset(new RootGroup());
    }

    void test_group_creation() {
        assert_false(root_group_->exists<TextureRG0>(0));
        assert_false(root_group_->exists<ShaderGroup>(0));

        root_group_->get_or_create<TextureRG0>(0).get_or_create<TextureRG1>(1);
        root_group_->get_or_create<ShaderGroup>(0);

        assert_true(root_group_->exists<TextureRG0>(0));
        assert_true(root_group_->get<TextureRG0>(0).exists<TextureRG1>(1));
        assert_false(root_group_->get<TextureRG0>(0).exists<TextureRG1>(2));
        assert_true(root_group_->exists<ShaderGroup>(0));
    }

private:
    kglt::RootGroup::ptr root_group_;
};

#endif // TEST_BATCHER_H
