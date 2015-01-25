#ifndef GENERIC_TREE_H
#define GENERIC_TREE_H

#include <list>
#include <kazbase/signals.h>

class GenericTreeNode;

typedef sig::signal<void (GenericTreeNode*, GenericTreeNode*)> ParentChangedSignal;

class GenericTreeNode {
public:
    GenericTreeNode();
    GenericTreeNode(GenericTreeNode* parent);

    virtual ~GenericTreeNode();

    const std::list<GenericTreeNode*> siblings() const;
    const std::list<GenericTreeNode*> children() const;

    bool is_root() const;
    bool has_parent() const;
    void set_parent(GenericTreeNode* node);
    GenericTreeNode* parent() const;

    void detach();
    void detach_children();

    GenericTreeNode* first_child() const;
    GenericTreeNode* last_child() const;

    GenericTreeNode* left_sibling() const;
    GenericTreeNode* right_sibling() const;

    template<typename T>
    T* as() {
        return static_cast<T*>(this);
    }

    ParentChangedSignal& signal_parent_changed() { return signal_parent_changed_; }

    void apply_recursively(std::function<void (GenericTreeNode*)> func, bool include_this=true);
    void apply_recursively_leaf_first(std::function<void (GenericTreeNode*)> func, bool include_this=true);
private:
    GenericTreeNode* parent_;
    std::list<GenericTreeNode*> children_;

    GenericTreeNode* left_;
    GenericTreeNode* right_;

    ParentChangedSignal signal_parent_changed_;
};


#endif // GENERIC_TREE_H
