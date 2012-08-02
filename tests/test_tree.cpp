#include <unittest++/UnitTest++.h>

#include <vector>

#include "kglt/shortcuts.h"
#include "kglt/kglt.h"
#include "kglt/object.h"
#include "kglt/generic/tree.h"

using namespace kglt;

TEST(test_tree_basic_usage) {
    class Object : public generic::TreeNode<Object> {};

    Object root;
    CHECK(!root.has_parent());
    CHECK(!root.has_children());
    CHECK(!root.child_count());

    Object child1;
    child1.set_parent(root);

    CHECK(child1.has_parent());
    CHECK(root.has_children());
    CHECK(!child1.has_children());
    CHECK(root.child_count());

    Object& c = root.child(0); //Get a reference to child1

    c.detach();
    CHECK(!root.has_children());
    CHECK(!c.has_parent());
}

TEST(test_tree_iteration) {
    class Object : public generic::TreeNode<Object> {};

    Object root, node1, node2, node3;

    node1.set_parent(root);
    node2.set_parent(root);
    node3.set_parent(node1);

    generic::tree_iterator<Object> iter(root);
    generic::tree_iterator<Object> end;

    uint32_t i = 0;
    for(; iter != end; ++iter) {
        Object& o = (*iter);

        if(i == 0) {
            CHECK(!o.has_parent()); //First one is the root
        } else {
            CHECK(o.has_parent()); //All the others aren't
        }
        ++i;
    }
    CHECK_EQUAL(4, i);
}
