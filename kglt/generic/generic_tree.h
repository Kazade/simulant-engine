#ifndef GENERIC_TREE_H
#define GENERIC_TREE_H

#include <list>

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

private:
    GenericTreeNode* parent_;
    std::list<GenericTreeNode*> children_;

    GenericTreeNode* left_;
    GenericTreeNode* right_;
};


#endif // GENERIC_TREE_H
