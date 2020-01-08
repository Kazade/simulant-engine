#pragma once

#include <functional>
#include <cstdint>
#include <iterator>

#include "../macros.h"
#include "../generic/check_signature.h"

/* Basic tree node class with sibling and parent/child access */

namespace smlt {


class TreeNode {
public:
    TreeNode();
    virtual ~TreeNode();

    void set_parent(TreeNode* node);
    void add_child(TreeNode* node);
    void remove_from_parent();

    bool is_root() const { return !parent_; }
    bool is_leaf() const { return first_child_ == nullptr; }
    TreeNode* parent() const { return parent_; }

    uint32_t child_count() const;

    TreeNode* first_child() const { return first_child_; }
    TreeNode* next_node() const { return right_; }
    TreeNode* previous_node() const { return left_; }

protected:
    TreeNode* root_ = nullptr;
    TreeNode* parent_ = nullptr;
    TreeNode* first_child_ = nullptr;

    TreeNode* left_ = nullptr;
    TreeNode* right_ = nullptr;

    TreeNode* detach(); //< Detach from all other nodes, return the first orphaned child (if any)
    TreeNode* last_child() const;

    virtual void on_parent_set(TreeNode* oldp, TreeNode* newp) {
        _S_UNUSED(oldp);
        _S_UNUSED(newp);
    }
};


}
