//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//


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
    GenericTreeNode* old_parent = parent();

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

    if(old_parent != parent_) {
        signal_parent_changed_(old_parent, parent_);
    }
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

    left_ = right_ = nullptr;

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

void GenericTreeNode::apply_recursively_leaf_first(std::function<void (GenericTreeNode*)> func, bool include_this) {
    auto copy = children_;
    for(auto child: copy) {
        child->apply_recursively_leaf_first(func, true);
    }

    if(include_this) {
        func(this);
    }
}

void GenericTreeNode::apply_recursively(std::function<void (GenericTreeNode*)> func, bool include_this) {
    if(include_this) {
        func(this);
    }

    auto copy = children_;
    for(auto child: copy) {
        child->apply_recursively(func, true);
    }
}
