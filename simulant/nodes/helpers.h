#pragma once

namespace smlt {
class StageNode;
struct WithBase {
    StageNode* base;
    WithBase(StageNode* base) :
        base(base) {}
};

} // namespace smlt
