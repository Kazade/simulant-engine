#ifndef TEST_GENERIC_TREE_H
#define TEST_GENERIC_TREE_H

#include "kaztest/kaztest.h"

#include "simulant/generic/generic_tree.h"

class GenericTreeTest : public TestCase {
public:
    void test_parent_set() {

        GenericTreeNode root, child1, child2;

        assert_false(child1.has_parent());
        child1.set_parent(&root);
        assert_true(child1.has_parent());
        assert_equal(child1.parent(), &root);
        assert_equal(1u, root.children().size());
        assert_equal(0u, child1.siblings().size());

        child2.set_parent(&root);
        assert_equal(1u, child1.siblings().size());
        assert_equal(1u, child2.siblings().size());
        assert_equal(2u, root.children().size());

        child1.detach();

        assert_false(child1.has_parent());
        assert_is_null(child1.parent());
        assert_equal(1u, root.children().size());
        assert_equal(0u, child1.siblings().size());
    }

};

#endif // TEST_GENERIC_TREE_H
