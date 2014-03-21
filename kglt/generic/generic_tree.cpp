
#include "generic_tree.h"

GenericTreeNode::GenericTreeNode():
    parent_(nullptr),
    left_(nullptr),
    right_(nullptr) {

}

GenericTreeNode::GenericTreeNode(GenericTreeNode* parent):
    parent_(parent),
    left_(nullptr),
    right_(nullptr) {

}

GenericTreeNode::~GenericTreeNode() {
    detach();
    detach_children();
}

GenericTreeNode* GenericTreeNode::first_child() const {
    if(children_.empty()) return nullptr;

    return children_.front();
}

GenericTreeNode* GenericTreeNode::last_child() const {
    if(children_.empty()) return nullptr;

    return children_.back();
}

GenericTreeNode* GenericTreeNode::left_sibling() const {
    return left_;
}

GenericTreeNode* GenericTreeNode::right_sibling() const {
    return right_;
}

const std::list<GenericTreeNode*> GenericTreeNode::siblings() const {
    std::list<GenericTreeNode*> result;
    if(!has_parent()) {
        return result;
    }

    result = parent_->children_;
    result.remove((GenericTreeNode*)this); //FIXME: C++ cast!
    return result;
}

const std::list<GenericTreeNode *> GenericTreeNode::children() const {
    return children_;
}

bool GenericTreeNode::has_parent() const {
    return parent() != nullptr;
}

void GenericTreeNode::set_parent(GenericTreeNode* node) {
    detach();

    if(!node) {
        //Setting to nullptr? Do nothing else
        return;
    }

    GenericTreeNode* left_sibling = node->last_child();

    if(left_sibling) {
        left_sibling->right_ = this;
        this->left_ = left_sibling;
    }

    this->parent_ = node;
    this->parent_->children_.push_back(this);
}

GenericTreeNode* GenericTreeNode::parent() const {
    return parent_;
}

void GenericTreeNode::detach() {
    if(left_) {
        left_->right_ = right_;
    }

    if(right_) {
        right_->left_ = left_;
    }

    if(has_parent()) {
        parent_->children_.remove(this);
        parent_ = nullptr;
    }
}

void GenericTreeNode::detach_children() {
    //We copy the list as child->detach manipulates parent->children_
    std::list<GenericTreeNode*> children = children_;
    for(GenericTreeNode* child: children) {
        child->detach();
    }
}
