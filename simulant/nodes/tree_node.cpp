
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

uint32_t TreeNode::child_count() const {
    uint32_t ret = 0;
    auto child = first_child_;
    if(!child) {
        return 0;
    } else {
        ret++;
        child = child->next_;
    }

    while(child != first_child_) {
        ret++;
        child = child->next_;
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

    auto old = node->parent_;

    if(node->parent_) {
        node->remove_from_parent();
    }

    node->parent_ = this;
    node->root_ = this->root_;

    if(!first_child_) {
        first_child_ = node;
        node->next_ = node->prev_ = node;
    } else {
        /* Insert to the end of the list (e.g. before the first child)
         * in the circular list */
        node->prev_ = first_child_->prev_;
        node->next_ = first_child_;

        node->next_->prev_ = node;
        node->prev_->next_ = node;
    }

    node->on_parent_set(old, node->parent_);
}

void TreeNode::remove_from_parent() {
    if(!parent_) {
        return;
    }

    /* Detach from parent */
    if(parent_->first_child_ == this) {
        parent_->first_child_ = next_;
        if(next_ == this) {
            parent_->first_child_ = nullptr;
        }
    }

    /* Remove from the sibling list */
    prev_->next_ = next_;
    next_->prev_ = prev_;

    /* Set ourselves to point only to ourselves */
    next_ = prev_ = this;

    /* We are now the root of our tree */
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

    return first_child_->prev_;
}

}
