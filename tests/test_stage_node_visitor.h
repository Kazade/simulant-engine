#pragma once

#include <set>

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/nodes/frustum_culler.h"
#include "simulant/nodes/stage_node_visitors.h"

namespace {

using namespace smlt;

class StageNodeVisitorBFSTests : public smlt::test::SimulantTestCase {
public:

    void test_visits_all_nodes() {
        /*
            root
           /    \
          a1     a2
         /  \      \
        b1   b2     b3
        */

        auto root = scene->create_child<smlt::Stage>();
        auto a1 = scene->create_child<smlt::Stage>();
        auto a2 = scene->create_child<smlt::Stage>();
        auto b1 = scene->create_child<smlt::Stage>();
        auto b2 = scene->create_child<smlt::Stage>();
        auto b3 = scene->create_child<smlt::Stage>();

        a1->set_parent(root);
        a2->set_parent(root);
        b1->set_parent(a1);
        b2->set_parent(a1);
        b3->set_parent(a2);

        std::set<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            visited.insert(node);
        });

        std::set<StageNode*> expected{root, a1, a2, b1, b2, b3};
        assert_items_equal(visited, expected);
    }

    void test_visits_single_node() {
        auto root = scene->create_child<smlt::Stage>();

        std::set<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            visited.insert(node);
        });

        std::set<StageNode*> expected{root};
        assert_items_equal(visited, expected);
    }

    void test_visits_large_tree_without_overflow() {
        /* Create a tree with well more than 128 nodes at one level */
        auto root = scene->create_child<smlt::Stage>();
        std::vector<StageNode*> children;

        /* Create 200 direct children of root */
        for (int i = 0; i < 200; ++i) {
            auto child = scene->create_child<smlt::Stage>();
            child->set_parent(root);
            children.push_back(child);
        }

        /* Add some grandchildren */
        for (int i = 0; i < 50; ++i) {
            auto grandchild = scene->create_child<smlt::Stage>();
            grandchild->set_parent(children[i]);
        }

        std::set<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            visited.insert(node);
        });

        /* Should visit root + 200 children + 50 grandchildren = 251 */
        assert_equal(251u, visited.size());
        assert_true(visited.count(root));
        for (auto* child : children) {
            assert_true(visited.count(child));
        }
    }

    void test_bfs_order_level_by_level() {
        /*
            root
           /    \
          a1     a2
         /         \
        b1          b2
        */

        auto root = scene->create_child<smlt::Stage>();
        auto a1 = scene->create_child<smlt::Stage>();
        auto a2 = scene->create_child<smlt::Stage>();
        auto b1 = scene->create_child<smlt::Stage>();
        auto b2 = scene->create_child<smlt::Stage>();

        a1->set_parent(root);
        a2->set_parent(root);
        b1->set_parent(a1);
        b2->set_parent(a2);

        std::vector<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            visited.push_back(node);
        });

        assert_equal(5u, visited.size());

        /* Root must be first */
        assert_equal(root, visited[0]);

        /* a1 and a2 must come before b1 and b2 (BFS property) */
        auto a1_pos = std::find(visited.begin(), visited.end(), a1) - visited.begin();
        auto a2_pos = std::find(visited.begin(), visited.end(), a2) - visited.begin();
        auto b1_pos = std::find(visited.begin(), visited.end(), b1) - visited.begin();
        auto b2_pos = std::find(visited.begin(), visited.end(), b2) - visited.begin();

        assert_true(a1_pos < b1_pos);
        assert_true(a2_pos < b2_pos);
    }

    void test_early_termination() {
        /*
            root
           /    \
          a1     a2
        */

        auto root = scene->create_child<smlt::Stage>();
        auto a1 = scene->create_child<smlt::Stage>();
        auto a2 = scene->create_child<smlt::Stage>();

        a1->set_parent(root);
        a2->set_parent(root);

        /* Collect only the first 2 nodes via a counter guard in the callback */
        std::vector<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            if(visited.size() < 2) {
                visited.push_back(node);
            }
        });

        assert_equal(2u, visited.size());
        /* Root is always first in BFS */
        assert_equal(root, visited[0]);
    }

    void test_deep_tree() {
        /* Create a deep tree (linear chain) to test depth handling */
        auto root = scene->create_child<smlt::Stage>();
        auto* current = root;

        std::vector<StageNode*> expected;
        expected.push_back(root);

        /* Create a chain of 300 nodes */
        for (int i = 0; i < 300; ++i) {
            auto child = scene->create_child<smlt::Stage>();
            child->set_parent(current);
            current = child;
            expected.push_back(child);
        }

        std::set<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            visited.insert(node);
        });

        assert_equal(301u, visited.size());
        for (auto* node : expected) {
            assert_true(visited.count(node));
        }
    }

    void test_does_not_descend_into_renderable_generating_nodes() {
        /* Regression test: nodes that generate renderables on behalf of their
         * whole subtree (FrustumCuller and other Partitioner subclasses,
         * ShadowCaster) signal this via generates_renderables_for_descendents()
         * returning true. traverse_bfs must stop descending into such a node's
         * children -- otherwise the compositor would call generate_renderables()
         * on those same descendants a second time, once via the node's own
         * generation pass and once via traverse_bfs visiting them directly.

            root
           /    \
          culler  a2
           |
           b1
        */
        auto root = scene->create_child<smlt::Stage>();
        auto culler = scene->create_child<smlt::FrustumCuller>();
        auto a2 = scene->create_child<smlt::Stage>();
        auto b1 = scene->create_child<smlt::Stage>();

        culler->set_parent(root);
        a2->set_parent(root);
        b1->set_parent(culler);

        assert_true(culler->generates_renderables_for_descendents());

        std::set<StageNode*> visited;
        traverse_bfs(root, [&](StageNode* node) {
            visited.insert(node);
        });

        /* culler itself must still be visited (so it gets a chance to
         * generate renderables for its subtree), but b1 -- its descendant --
         * must not be visited directly. */
        std::set<StageNode*> expected{root, culler, a2};
        assert_items_equal(visited, expected);
        assert_false(visited.count(b1));
    }
};

}
