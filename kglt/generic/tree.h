#ifndef TREE_H
#define TREE_H

#include <boost/iterator/iterator_facade.hpp>
#include <stdexcept>
#include <vector>
#include <tr1/memory>
#include <sigc++/sigc++.h>

#include "kazbase/logging/logging.h"

namespace kglt {
namespace generic {

class InvalidParentNodeError : public std::logic_error {
public:
    InvalidParentNodeError():
        std::logic_error("Tried to set an invalid parent type on a tree node") {}
};

template<typename T>
class tree_iterator :
        public boost::iterator_facade<
            tree_iterator<T>,
            T,
            boost::forward_traversal_tag,
            T&
        > {

public:
    tree_iterator():
        position_(0) {
    }

    explicit tree_iterator(T& p):
        position_(0) {
        linearized_tree_ = linearize_tree(&p);
    }

private:
    friend class boost::iterator_core_access;

    void linearize_recurse(T* node, std::vector<T*>& results) {
        results.push_back(node);
        for(uint32_t i = 0; i < node->child_count(); ++i) {
            linearize_recurse(&node->child(i), results);
        }
    }

    std::vector<T*> linearize_tree(T* start) {
        std::vector<T*> results;
        linearize_recurse(start, results);
        return results;
    }

    std::vector<T*> linearized_tree_;
    uint32_t position_;

    void increment() {
        ++position_;

        if(position_ == linearized_tree_.size()) {
            position_ = 0;
            linearized_tree_.clear();
        }
    }

    bool equal(tree_iterator<T> const& other) const {
        return this->linearized_tree_ == other.linearized_tree_ &&
                this->position_ == other.position_;
    }

    T& dereference() const {
        return *linearized_tree_.at(position_);
    }
};

template<typename T>
class TreeNode {
public:
    typedef tree_iterator<TreeNode<T> > iterator;

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

    void set_parent(T* new_parent) {
        if(!can_set_parent(new_parent)) {
            throw InvalidParentNodeError();
        }

        T* old_parent = (has_parent()) ? &parent() : nullptr;

        if(old_parent == new_parent) {
            return; //Do nothing if we are setting the same parent
        }

        if(old_parent) {
            old_parent->detach_child((T*)this);
        }

        if(new_parent) {
            new_parent->attach_child((T*)this);
        }

        on_parent_set(old_parent);

        signal_parent_changed_(old_parent, new_parent); //Fire off a signal to indicate that the parent changed
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
        T* self = (T*)this;
        while(self->has_parent()) {
            self = &self->parent();
        }

        return dynamic_cast<const Derived&>(*self);
    }

    template<typename Derived>
    Derived& parent_as() {
        assert(has_parent());
        return dynamic_cast<Derived&>(parent());
    }

    virtual void on_parent_set(T* old_parent) {}


    sigc::signal<void, T*, T*>& signal_parent_changed() { return signal_parent_changed_; }

    tree_iterator<TreeNode<T> > begin() { return tree_iterator<TreeNode<T>>(*this); }
    tree_iterator<TreeNode<T> > end() { return tree_iterator<TreeNode<T> > (); }

    bool has_siblings() const { return has_parent() ? parent().child_count() > 1 : false; }

protected:
    std::vector<T*>& children() { return children_; }

private:
    T* parent_;
    std::vector<T*> children_;

    sigc::signal<void, T*, T*> signal_parent_changed_;

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

    virtual bool can_set_parent(T* parent) {
        return true;
    }
};

} //namespace generic
} //namespace kglt

#endif // TREE_H
