
#include "tree_node.h"

namespace smlt {

TreeNode::TreeNode():
    root_(this) {

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

    if(left_) {
        left_->right_ = right_;
        left_ = nullptr;
    } else {
        parent_->first_child_ = right_;
    }

    if(right_) {
        right_->left_ = left_;
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


void TreeNode::each_child(TreeNodeTraversalCallback cb) {
    uint32_t i = 0;
    TreeNode* child = first_child_;
    while(child) {
        cb(i++, child);
        child = child->right_;
    }
}

void TreeNode::each_sibling(TreeNodeTraversalCallback cb) {
    uint32_t i = 0;
    TreeNode* sibling = parent_->first_child_;
    while(sibling) {
        if(sibling != this) {
            cb(i++, sibling);
        }
        sibling = sibling->right_;
    }
}

void TreeNode::each_ancestor(TreeNodeTraversalCallback cb) {
    uint32_t i = 0;
    TreeNode* ancestor = parent_;
    while(ancestor) {
        cb(i++, ancestor);
        ancestor = ancestor->parent_;
    }
}

void TreeNode::each_descendent(TreeNodeTraversalCallback cb) {
    TreeNode* descendent = first_child_;
    while(descendent) {
        cb(0, descendent); //FIXME: Increment first argument for each descendent

        descendent->each_descendent(cb);
        descendent = descendent->right_;
    }
}

void TreeNode::each_descendent_and_self(TreeNodeTraversalCallback cb) {
    cb(0, this);
    each_descendent(cb); // FIXME: Need increment to start from 1
}

// Leaf-first versions
void TreeNode::each_descendent_lf(TreeNodeTraversalCallback cb) {
    TreeNode* descendent = first_child_;
    while(descendent) {
        descendent->each_descendent_lf(cb);
        cb(0, descendent); //FIXME: Increment first argument for each descendent
        descendent = descendent->right_;
    }
}

void TreeNode::each_descendent_and_self_lf(TreeNodeTraversalCallback cb) {
    each_descendent_lf(cb);
    cb(0, this); //FIXME: Should not be zero
}

}
