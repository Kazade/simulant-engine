#ifndef TREE_H
#define TREE_H

#include <stdexcept>
#include <vector>
#include <tr1/memory>

#include "kazbase/logging/logging.h"

class InvalidParentNodeError : public std::logic_error {
public:
    InvalidParentNodeError():
        std::logic_error("Tried to set an invalid parent type on a tree node") {}
};

template<typename T>
class TreeNode {
public:
    TreeNode():
        parent_(nullptr) {}

    virtual ~TreeNode() {
        try {
            detach();
        } catch(...) {
            L_ERROR("Exception while detaching a tree node during destruction");
        }
    }

    void set_parent(T& parent) {
        set_parent(&parent);
    }

    void set_parent(T* p) {
        if(!can_set_parent(p)) {
            throw InvalidParentNodeError();
        }

        T* old_parent = nullptr;
        if(has_parent()) {
            old_parent = &parent();
            parent().detach_child((T*)this);
        }

        if(p) {
            p->attach_child((T*)this);
        }
    }

    void detach() {
        set_parent(nullptr);
    }

    uint32_t child_count() const { return children_.size(); }
    T& child(uint32_t idx) { return *children_.at(idx); }

    bool has_parent() const { return parent_ != nullptr; }
    bool has_children() const { return !children_.empty(); }

    T& parent() { assert(parent_); return *parent_; }
    const T& parent() const { assert(parent_); return *parent_; }

    template<typename Derived>
    Derived& root_as() {
        return const_cast<Derived&>(static_cast<const T*>(this)->root_as<Derived>());
    }

    template<typename Derived>
    const Derived& root_as() const {
        T* self = this;
        while(self->has_parent()) {
            self = &self->parent();
        }

        return dynamic_cast<const Derived&>(*self);
    }

private:
    T* parent_;
    std::vector<T*> children_;

    void attach_child(T* child) {
        if(child->has_parent()) {
            child->parent().detach_child(child);
        }

        child->parent_ = (T*)this;
        children_.push_back(child);
    }

    void detach_child(T* child) {
        //Make sure that the child has a parent
        if(!child->has_parent()) {
            return;
        }

        //Make sure *we* are the parent
        if(&child->parent() != this) {
            return;
        }

        //Erase the child from our children and set its parent to null
        child->parent_ = nullptr;
        children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
    }

    virtual bool can_set_parent(T* parent) { return true; }
};

#endif // TREE_H
