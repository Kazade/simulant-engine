#include <unittest++/UnitTest++.h>

#include <vector>

#include "kglt/shortcuts.h"
#include "kglt/kglt.h"
#include "kglt/object.h"
#include "kglt/generic/tree.h"

using namespace kglt;

TEST(test_tree_basic_usage) {
    class Object : public TreeNode<Object> {};

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


