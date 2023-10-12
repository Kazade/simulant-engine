#pragma once

#include <simulant/test.h>
#include <simulant/simulant.h>

namespace {

using namespace smlt;

class MixinTests : public test::SimulantTestCase {
public:
    void test_basic_usage() {
        auto node = scene->create_node<Stage>();
        auto mixin1 = node->create_mixin<FlyController>();
        mixin1->set_name("Mixin");

        // Mixins don't have parents
        assert_is_null(mixin1->parent());
        assert_equal(mixin1->base(), node); // But they do have a base node
        assert_equal(node->base(), node); // Non-mixins have a base node of themselves
        assert_false(node->is_mixin());

        // Mixins can't have mixins
        assert_is_null(mixin1->create_mixin<Stage>());

        // You can't re-parent mixins, and mixins don't have parents
        mixin1->set_parent(node);
        assert_is_null(mixin1->parent());

        assert_equal(node->mixin_count(), 1u);
        assert_true(mixin1->is_mixin());
        assert_equal(mixin1->transform.get(), node->transform.get());

        auto mixin2 = node->find_mixin("Mixin");
        assert_is_not_null(mixin2);
        assert_equal(mixin1, mixin2);

        // Mixins should have their update functions called
        int update_called = 0;
        int late_update_called = 0;
        int fixed_update_called = 0;

        mixin1->signal_update().connect([&](float) { update_called++; });
        mixin1->signal_late_update().connect([&](float) { late_update_called++; });
        mixin1->signal_fixed_update().connect([&](float) { fixed_update_called++; });

        application->run_frame();

        assert_equal(update_called, 1);
        assert_equal(late_update_called, 1);
        assert_true(fixed_update_called > 0);

        // Don't allow creating the same mixin twice
        assert_is_null(node->create_mixin<FlyController>());

        mixin1->destroy();

        assert_equal(node->mixin_count(), 0u);
        assert_is_null(node->find_mixin("Mixin"));

        // Can create it now, as the other one was destroyed
        assert_is_not_null(node->create_mixin<FlyController>());
    }
};

}
