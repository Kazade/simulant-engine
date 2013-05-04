#ifndef TEST_TREE_H
#define TEST_TREE_H

#include "kglt/kazbase/testing.h"

#include "kglt/kglt.h"
#include "global.h"

class TreeTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_tree_basic_usage() {
        class Object : public kglt::generic::TreeNode<Object> {};

        Object root;
        assert_false(root.has_parent());
        assert_false(root.has_children());
        assert_false(root.child_count());

        Object child1;
        child1.set_parent(root);

        assert_true(child1.has_parent());
        assert_true(root.has_children());
        assert_false(child1.has_children());
        assert_true(root.child_count());

        Object& c = root.child(0); //Get a reference to child1

        c.detach();
        assert_false(root.has_children());
        assert_false(c.has_parent());
    }

    void test_tree_iteration() {
        class Object : public kglt::generic::TreeNode<Object> {};

        Object root, node1, node2, node3;

        node1.set_parent(root);
        node2.set_parent(root);
        node3.set_parent(node1);

        assert_true(node1.has_siblings());
        assert_true(node2.has_siblings());
        assert_false(node3.has_siblings());

        kglt::generic::tree_iterator<Object> iter(root);
        kglt::generic::tree_iterator<Object> end;

        int32_t i = 0;
        for(; iter != end; ++iter) {
            Object& o = (*iter);

            if(i == 0) {
                assert_false(o.has_parent()); //First one is the root
            } else {
                assert_true(o.has_parent()); //All the others aren't
            }
            ++i;
        }
        assert_equal(4, i);
    }

};

#endif // TEST_TREE_H
