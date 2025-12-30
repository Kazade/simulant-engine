/*
 * This file is included at the bottom of stage_node.h due to template
 * suckiness
 */

namespace smlt {

template<bool IsConst>
class SiblingIterator;

template<bool IsConst>
SiblingIterator<IsConst>& SiblingIterator<IsConst>::operator++() {
    if(current_ && current_->next_sibling() != start_) {
        current_ = (StageNode*) current_->next_sibling();
    } else {
        current_ = nullptr;
    }

    return *this;
}

template<bool IsConst>
SiblingIterator<IsConst>::SiblingIterator(const StageNode *start, const StageNode* current):
    start_(start),
    current_(current) {

    assert(!current_ || start_->parent() == current_->parent());
}

}
