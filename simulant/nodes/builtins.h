#pragma once

#include <cstdint>

namespace smlt {

typedef uint32_t StageNodeType;

// Declares how a stage node class is intended to be used - as freestanding
// scene content (created directly, e.g. via create_child()), as an add-on
// attached to an existing node (created via create_mixin(), sharing its
// base's transform - e.g. Gizmo, StaticBody), or either. Set via
// S_DEFINE_STAGE_NODE_META's optional second argument; defaults to
// STAGE_NODE_USAGE_EITHER when omitted. Purely declarative - it isn't
// enforced by create_child()/create_mixin() themselves, but lets tooling
// (e.g. Simulant Studio's "create node" dialog) filter out types that
// don't make sense to place directly in the scene.
enum StageNodeUsage {
    STAGE_NODE_USAGE_EITHER,
    STAGE_NODE_USAGE_NODE_ONLY,
    STAGE_NODE_USAGE_MIXIN_ONLY,
};

}
