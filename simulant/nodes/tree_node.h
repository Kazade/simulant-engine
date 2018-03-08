#pragma once

#include <functional>
#include <cstdint>

#include "../generic/check_signature.h"

/* Basic tree node class with sibling and parent/child access */

namespace smlt {

class TreeNode;

class TreeNode {
public:
    using TraversalCallback = void(uint32_t, TreeNode*);

    TreeNode();
    virtual ~TreeNode() {}

    void set_parent(TreeNode* node);
    void add_child(TreeNode* node);
    void remove_from_parent();

    bool is_root() const { return !parent_; }
    bool is_leaf() const { return first_child_ == nullptr; }
    TreeNode* parent() const { return parent_; }

    template <typename Callback>
    void each_child(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        uint32_t i = 0;
        TreeNode* child = first_child_;
        while(child) {
            cb(i++, child);
            child = child->right_;
        }
    }

    template <typename Callback>
    void each_sibling(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        uint32_t i = 0;
        TreeNode* sibling = parent_->first_child_;
        while(sibling) {
            if(sibling != this) {
                cb(i++, sibling);
            }
            sibling = sibling->right_;
        }
    }

    template<typename Callback>
    void each_ancestor(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        uint32_t i = 0;
        TreeNode* ancestor = parent_;
        while(ancestor) {
            cb(i++, ancestor);
            ancestor = ancestor->parent_;
        }
    }

    template<typename Callback>
    void each_descendent(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        TreeNode* descendent = first_child_;
        while(descendent) {
            cb(0, descendent); //FIXME: Increment first argument for each descendent

            descendent->each_descendent(cb);
            descendent = descendent->right_;
        }
    }

    template<typename Callback>
    void each_descendent_and_self(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        cb(0, this);
        each_descendent(cb); // FIXME: Need increment to start from 1
    }

    // Leaf-first versions
    template<typename Callback>
    void each_descendent_lf(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        TreeNode* descendent = first_child_;
        while(descendent) {
            descendent->each_descendent_lf(cb);
            auto next = descendent->right_;
            cb(0, descendent); //FIXME: Increment first argument for each descendent
            descendent = next;
        }
    }

    template<typename Callback>
    void each_descendent_and_self_lf(const Callback& cb) {
        check_signature<Callback, TraversalCallback>();

        each_descendent_lf(cb);
        cb(0, this); //FIXME: Should not be zero
    }

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
