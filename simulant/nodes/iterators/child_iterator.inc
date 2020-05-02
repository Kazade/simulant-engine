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
        current_ = (StageNode*) current_->next_node();
    }

    if(current_ == parent_->first_child()) {
        current_ = nullptr;
    }

    return *this;
}

template<bool IsConst>
ChildIterator<IsConst>::ChildIterator(StageNode *parent):
    parent_(parent),
    current_((StageNode*) parent_->first_child()) {

}

template<bool IsConst>
ChildIterator<IsConst>::ChildIterator(StageNode *parent, StageNode* start):
    parent_(parent),
    current_(start) {

    assert(!current_ || current_->parent() == parent);
}

}
