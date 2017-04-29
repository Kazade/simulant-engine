#pragma once

#include <functional>

/* Basic tree node class with sibling and parent/child access */

namespace smlt {

class TreeNode;

typedef std::function<void (uint32_t, TreeNode*)> TreeNodeTraversalCallback;

class TreeNode {
public:
    TreeNode();
    virtual ~TreeNode() {}

    void set_parent(TreeNode* node);
    void add_child(TreeNode* node);
    void remove_from_parent();

    bool is_root() const { return !parent_; }
    bool is_leaf() const { return first_child_ == nullptr; }
    TreeNode* parent() const { return parent_; }

    void each_child(TreeNodeTraversalCallback cb);
    void each_sibling(TreeNodeTraversalCallback cb);
    void each_ancestor(TreeNodeTraversalCallback cb);
    void each_descendent(TreeNodeTraversalCallback cb);
    void each_descendent_and_self(TreeNodeTraversalCallback cb);

    // Leaf-first versions
    void each_descendent_lf(TreeNodeTraversalCallback cb);
    void each_descendent_and_self_lf(TreeNodeTraversalCallback cb);

    uint32_t count_children() const;
private:
    TreeNode* root_ = nullptr;
    TreeNode* parent_ = nullptr;
    TreeNode* first_child_ = nullptr;

    TreeNode* left_ = nullptr;
    TreeNode* right_ = nullptr;

protected:
    TreeNode* detach(); //< Detach from all other nodes, return the first orphaned child (if any)

    TreeNode* last_child() const;

    virtual void on_parent_set(TreeNode* oldp, TreeNode* newp) {}
};


}
