#pragma once

#include "stage_node.h"

namespace smlt {

class StageNode;

class SiblingIteratorPair {
    friend class StageNode;

    SiblingIteratorPair(const StageNode* root) :
        root_(root) {}

    const StageNode* root_;

public:
    SiblingIterator<false> begin() {
        if(!root_->parent()) {
            return SiblingIterator<false>(root_, nullptr);
        }

        return SiblingIterator<false>(
            root_, (root_->next_sibling()) ? root_->next_sibling()
                                           : root_->parent_->first_child());
    }

    SiblingIterator<false> end() {
        return SiblingIterator<false>(root_, nullptr);
    }
};

class ChildIteratorPair {
    friend class StageNode;

    ChildIteratorPair(const StageNode* root) :
        root_(root) {}

    const StageNode* root_;

public:
    ChildIterator<false> begin() const {
        return ChildIterator<false>(root_, root_->first_child_);
    }

    ChildIterator<false> end() const {
        return ChildIterator<false>(root_, nullptr);
    }
};

class DescendentIteratorPair {
    friend class StageNode;

    DescendentIteratorPair(const StageNode* root) :
        root_(root) {}

    const StageNode* root_;

public:
    DescendentIterator<false> begin() const {
        return DescendentIterator<false>(root_);
    }

    DescendentIterator<false> end() const {
        return DescendentIterator<false>(root_, nullptr);
    }
};

class AncestorIteratorPair {
    friend class StageNode;

    AncestorIteratorPair(const StageNode* root) :
        root_(root) {}

    const StageNode* root_;

public:
    AncestorIterator<false> begin() {
        return AncestorIterator<false>(root_);
    }

    AncestorIterator<false> end() {
        return AncestorIterator<false>(root_, nullptr);
    }
};

} // namespace smlt
