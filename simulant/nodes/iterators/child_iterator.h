#pragma once

#include <cstdint>
#include <iterator>

namespace smlt {

class StageNode;

template<bool IsConst>
class ChildIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = StageNode;
    using difference_type = uint32_t;

    using pointer = typename std::conditional<
        IsConst, const StageNode*, StageNode*
    >::type;

    using reference = typename std::conditional<
        IsConst, const StageNode&, StageNode&
    >::type;

    bool operator==(const ChildIterator& rhs) const {
        return parent_ == rhs.parent_ && current_ == rhs.current_;
    }

    bool operator!=(const ChildIterator& rhs) const {
        return !(*this == rhs);
    }

    ChildIterator& operator++();

    /* Non-const versions */
    template<bool _IsConst=IsConst>
    typename std::enable_if<!_IsConst, pointer>::type
    operator->() {
        return current_;
    }

    template<bool _IsConst=IsConst>
    typename std::enable_if<!_IsConst, reference>::type
    operator*() {
        return *current_;
    }

    /* Const versions */
    template<bool _IsConst=IsConst>
    typename std::enable_if<_IsConst, pointer>::type
    operator->() const {
        return current_;
    }

    template<bool _IsConst=IsConst>
    typename std::enable_if<_IsConst, reference>::type
    operator*() const {
        return *current_;
    }

private:
    friend class StageNode;

    ChildIterator(const StageNode* parent, const StageNode* start);

    const StageNode* parent_ = nullptr;
    pointer current_ = nullptr;
};

}
