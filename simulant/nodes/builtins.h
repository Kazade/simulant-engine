#pragma once

namespace smlt {

enum BuiltInStageNodeType {
    STAGE_NODE_TYPE_SCENE,
    STAGE_NODE_TYPE_STAGE,
    STAGE_NODE_TYPE_CAMERA,
    STAGE_NODE_TYPE_ACTOR,
    STAGE_NODE_TYPE_LIGHT,
    STAGE_NODE_TYPE_DIRECTIONAL_LIGHT,
    STAGE_NODE_TYPE_POINT_LIGHT,
    STAGE_NODE_TYPE_PARTICLE_SYSTEM,
    STAGE_NODE_TYPE_GEOM,
    STAGE_NODE_TYPE_MESH_INSTANCER,
    STAGE_NODE_TYPE_SKYBOX,
    STAGE_NODE_TYPE_SPRITE,
    STAGE_NODE_TYPE_WIDGET_BUTTON,
    STAGE_NODE_TYPE_WIDGET_LABEL,
    STAGE_NODE_TYPE_WIDGET_FRAME,
    STAGE_NODE_TYPE_WIDGET_PROGRESS_BAR,
    STAGE_NODE_TYPE_WIDGET_IMAGE,
    STAGE_NODE_TYPE_WIDGET_TEXT_ENTRY,
    STAGE_NODE_TYPE_WIDGET_KEYBOARD,
    STAGE_NODE_TYPE_WIDGET_KEYBOARD_PANEL,
    STAGE_NODE_TYPE_UI_MANAGER,
    STAGE_NODE_TYPE_DEBUG,
    STAGE_NODE_TYPE_PARTITIONER_FRUSTUM,
    STAGE_NODE_TYPE_PARTITIONER_SPATIAL_HASH,
    STAGE_NODE_TYPE_PHYSICS_STATIC_BODY,
    STAGE_NODE_TYPE_PHYSICS_RIGID_BODY,
    STAGE_NODE_TYPE_PHYSICS_KINEMATIC_BODY,
    STAGE_NODE_TYPE_STATS_PANEL,
    STAGE_NODE_TYPE_FLY_CONTROLLER,
    STAGE_NODE_TYPE_SMOOTH_FOLLOW,
    STAGE_NODE_TYPE_CYLINDRICAL_BILLBOARD,
    STAGE_NODE_TYPE_SPHERICAL_BILLBOARD,
    STAGE_NODE_TYPE_BUILT_IN_MAX,
    STAGE_NODE_TYPE_USER_BASE = 1000
};

typedef uint32_t StageNodeType;

}