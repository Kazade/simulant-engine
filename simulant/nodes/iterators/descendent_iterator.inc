/*
 * This file is included at the bottom of stage_node.h due to template
 * suckiness
 */

namespace smlt {

template<bool IsConst>
class DescendentIterator;

template<bool IsConst>
DescendentIterator<IsConst>& DescendentIterator<IsConst>::operator++() {
    auto next = (StageNode*) current_->next_node();
    auto next_is_current_ = next == current_;
    auto next_is_first_sibling = !next->parent() || next == next->parent()->first_child();
    auto current_is_first_sibling = !current_->parent() || current_ == current_->parent()->first_child();

    if(next_is_first_sibling) {
        /* We've just reached the end of the current siblings, so
         * go up, then see if we can go right */
        previous_ = current_;
        current_ = (StageNode*) current_->parent();
        if(current_ == start_) {
            current_ = nullptr;
        }
        return *this;
    }

    /* Go right, then down the left branch to the end */
    previous_ = current_;
    current_ = next;
    while(current_->first_child()) {
        current_ = (StageNode*) current_->first_child();
    }

    return *this;
}

template<bool IsConst>
DescendentIterator<IsConst>::DescendentIterator(StageNode *start):
    start_(start) {

    /* We go down the left of the tree to the far
     * bottom left node */
    current_ = (StageNode*) start->first_child();
    while(current_ && current_->first_child()) {
        current_ = (StageNode*) current_->first_child();
    }
}

template<bool IsConst>
DescendentIterator<IsConst>::DescendentIterator(StageNode *start, StageNode* current):
    start_(start),
    current_(current) {

}

}
