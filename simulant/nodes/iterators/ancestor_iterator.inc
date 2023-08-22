/*
 * This file is included at the bottom of stage_node.h due to template
 * suckiness
 */

namespace smlt {

template<bool IsConst>
class AncestorIterator;

template<bool IsConst>
AncestorIterator<IsConst>& AncestorIterator<IsConst>::operator++() {
    if(current_) {
        current_ = (StageNode*) current_->parent();
    }

    return *this;
}

template<bool IsConst>
AncestorIterator<IsConst>::AncestorIterator(const StageNode *start):
    start_(start),
    current_((StageNode*) start_->parent()) {

}

template<bool IsConst>
AncestorIterator<IsConst>::AncestorIterator(const StageNode *start, const StageNode* current):
    start_(start),
    current_(current) {

}

}
