
#include "tree_node.h"
#include "tree_node.h"
#include "tree_node.h"
#include "tree_node.h"

namespace smlt {

TreeNode::TreeNode():
    root_(this) {

}

TreeNode::~TreeNode() {
    detach();
}

uint32_t TreeNode::count_children() const {
    uint32_t ret = 0;
    auto child = first_child_;
    while(child) {
        ret++;
        child = child->right_;
    }

    return ret;
}

void TreeNode::set_parent(TreeNode *node) {
    if(node == parent_) {
        // Do nothing if this is already the parent
        return;
    }

    if(node == this) {
        // Do nothing if we tried to set the parent node to itself
        return;
    }

    if(node) {
        node->add_child(this);
    }
}

void TreeNode::add_child(TreeNode *node) {
    if(node->parent_ == this) {
        return;
    }

    if(node->parent_) {
        node->remove_from_parent();
    }

    auto old = node->parent_;
    node->parent_ = this;
    node->root_ = this->root_;

    if(!first_child_) {
        first_child_ = node;
    } else {
        node->left_ = last_child();
        node->left_->right_ = node;

        node->parent_ = this;
    }

    on_parent_set(old, node->parent_);
}

void TreeNode::remove_from_parent() {
    if(!parent_) {
        return;
    }

    auto left = left_;
    auto right = right_;

    if(left_) {
        left_->right_ = right;
        left_ = nullptr;
    } else {
        parent_->first_child_ = right;
    }

    if(right_) {
        right_->left_ = left;
        right_ = nullptr;
    }

    parent_ = nullptr;
    root_ = this;
}

TreeNode* TreeNode::detach() {
    remove_from_parent();

    TreeNode* child = first_child_;
    first_child_ = nullptr;

    return child;
}

TreeNode* TreeNode::last_child() const {
    if(!first_child_) {
        return nullptr;
    }

    TreeNode* iter = first_child_;
    while(iter->right_) {
        iter = iter->right_;
    }

    return iter;
}








// Leaf-first versions




}
