#pragma once

#include <cstdint>
#include <iterator>

namespace smlt {

class StageNode;

template<bool IsConst>
class AncestorIterator {
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

    bool operator==(const AncestorIterator& rhs) const {
        return current_ == rhs.current_;
    }

    bool operator!=(const AncestorIterator& rhs) const {
        return !(*this == rhs);
    }

    AncestorIterator& operator++();

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

    AncestorIterator(StageNode* start);
    AncestorIterator(StageNode* start, StageNode* current);

    StageNode* start_ = nullptr;
    StageNode* current_ = nullptr;
};

}
