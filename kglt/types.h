#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cstdint>
#include <string>
#include "colour.h"

#include "kazmath/vec3.h"
#include "kazmath/vec2.h"
#include "kazmath/vec4.h"

#include "generic/unique_id.h"

namespace kglt {

struct Vec4 : public kmVec4 {
    Vec4() {
        kmVec4Fill(this, 0, 0, 0, 1);
    }

    Vec4(const kmVec3& v, float w) {
        kmVec4Fill(this, v.x, v.y, v.z, w);
    }
};

struct Vec3 : public kmVec3 {
    Vec3() {
        kmVec3Zero(this);
    }

    Vec3(float x, float y, float z) {
        kmVec3Fill(this, x, y, z);
    }
};

struct Vec2 : public kmVec2 {
    Vec2() {
        x = 0.0;
        y = 0.0;
    }
};

enum MeshArrangement {
    MESH_ARRANGEMENT_POINTS,
    MESH_ARRANGEMENT_TRIANGLES,
    MESH_ARRANGEMENT_TRIANGLE_FAN,
    MESH_ARRANGEMENT_TRIANGLE_STRIP,
    MESH_ARRANGEMENT_LINES,
    MESH_ARRANGEMENT_LINE_STRIP
};

enum VertexAttribute {
    VERTEX_ATTRIBUTE_POSITION = 1,
    VERTEX_ATTRIBUTE_TEXCOORD_1 = 2,
    VERTEX_ATTRIBUTE_DIFFUSE = 4,
    VERTEX_ATTRIBUTE_NORMAL = 8
};

typedef UniqueID<0> MeshID;
typedef UniqueID<1> TextureID;
typedef UniqueID<2> CameraID;
typedef UniqueID<3> ShaderID;
typedef UniqueID<4> MaterialID;
typedef UniqueID<5> LightID;
typedef UniqueID<6> SceneGroupID;
typedef UniqueID<7> ViewportID;
typedef UniqueID<8> EntityID;

const CameraID DefaultCameraID = CameraID();

const std::string DEFAULT_MATERIAL_SCHEME = "default";

class Mesh;
class Entity;
class Light;
class Scene;
class Camera;
class SceneGroup;
class ShaderProgram;
class Viewport;
class Frustum;
class WindowBase;
class Material;

}

#endif // TYPES_H_INCLUDED
