/*
 * This file is included at the bottom of stage_node.h due to template
 * suckiness
 */

namespace smlt {

template<bool IsConst>
class DescendentIterator;

template<bool IsConst>
DescendentIterator<IsConst>& DescendentIterator<IsConst>::operator++() {
    auto next = current_->next_;

    if(!next) {
        /* We've just reached the end of the current siblings, so
         * go up, then see if we can go right */
        previous_ = current_;
        current_ = current_->parent_;
        if(current_ == start_) {
            current_ = nullptr;
        }
        return *this;
    } else {
        /* Go right, then down the left branch to the end */
        previous_ = current_;
        current_ = next;
        while(current_->first_child_) {
            current_ = current_->first_child_;
        }
    }

    return *this;
}

template<bool IsConst>
DescendentIterator<IsConst>::DescendentIterator(const StageNode* start):
    start_(start) {

    /* We go down the left of the tree to the far
     * bottom left node */
    current_ = (StageNode*) start->first_child_;
    while(current_ && current_->first_child_) {
        current_ = (StageNode*) current_->first_child_;
    }
}

template<bool IsConst>
DescendentIterator<IsConst>::DescendentIterator(const StageNode* start, pointer current):
    start_(start),
    current_(current) {

}

}
