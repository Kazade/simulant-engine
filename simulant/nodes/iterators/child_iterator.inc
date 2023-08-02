/*
 * This file is included at the bottom of stage_node.h due to template
 * suckiness
 */

namespace smlt {

template<bool IsConst>
class ChildIterator;

template<bool IsConst>
ChildIterator<IsConst>& ChildIterator<IsConst>::operator++() {
    if(current_) {
        current_ = current_->next_sibling();
    }

    return *this;
}

template<bool IsConst>
ChildIterator<IsConst>::ChildIterator(StageNode *parent, StageNode* start):
    parent_(parent),
    current_(start) {

    assert(!current_ || current_->parent() == parent);
}

}
