/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GENERIC_TREE_H
#define GENERIC_TREE_H

#include <list>
#include "../deps/kazsignal/kazsignal.h"

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
    GenericTreeNode* parent_ = nullptr;
    std::list<GenericTreeNode*> children_;

    GenericTreeNode* left_ = nullptr;
    GenericTreeNode* right_ = nullptr;

    ParentChangedSignal signal_parent_changed_;
};


#endif // GENERIC_TREE_H
