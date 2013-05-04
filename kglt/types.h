#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <lua.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include "colour.h"

#include <kazmath/vec3.h>
#include <kazmath/vec2.h>
#include <kazmath/vec4.h>

#include "generic/unique_id.h"

namespace kglt {

struct Vec4 : public kmVec4 {
    Vec4() {
        kmVec4Fill(this, 0, 0, 0, 1);
    }

    Vec4(float x, float y, float z, float w) {
        kmVec4Fill(this, x, y, z, w);
    }

    Vec4(const kmVec3& v, float w) {
        kmVec4Fill(this, v.x, v.y, v.z, w);
    }
};

struct Vec2 : public kmVec2 {
    Vec2() {
        x = 0.0;
        y = 0.0;
    }

    Vec2(float x, float y) {
        kmVec2Fill(this, x, y);
    }
};

struct Vec3 : public kmVec3 {
    Vec3() {
        kmVec3Zero(this);
    }

    Vec3(float x, float y, float z) {
        kmVec3Fill(this, x, y, z);
    }

    Vec3(const kmVec2& v2, float z) {
        kmVec3Fill(this, v2.x, v2.y, z);
    }

    Vec3(const kmVec3& v) {
        kmVec3Fill(this, v.x, v.y, v.z);
    }
};

enum BlendType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOUR,
    BLEND_ALPHA,
    BLEND_ONE_ONE_MINUS_ALPHA
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

enum AvailablePartitioner {
    PARTITIONER_NULL,
    PARTITIONER_OCTREE
};

enum LightType {
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_SPOT_LIGHT
};

enum RenderPriority {
    RENDER_PRIORITY_BACKGROUND = -100,
    RENDER_PRIORITY_DISTANT = -50,
    RENDER_PRIORITY_MAIN = 0,
    RENDER_PRIORITY_NEAR = 50,
    RENDER_PRIORITY_FOREGROUND = 100
};

const std::vector<RenderPriority> RENDER_PRIORITIES = {
    RENDER_PRIORITY_BACKGROUND,
    RENDER_PRIORITY_DISTANT,
    RENDER_PRIORITY_MAIN,
    RENDER_PRIORITY_NEAR,
    RENDER_PRIORITY_FOREGROUND
};

template<typename T>
class LuaClass {
public:
    static void export_to(lua_State& state) {
        T::do_lua_export(state);
    }
};


typedef UniqueID<0> MeshID;
typedef UniqueID<1> TextureID;
typedef UniqueID<2> CameraID;
typedef UniqueID<3> ShaderID;
typedef UniqueID<4> MaterialID;
typedef UniqueID<5> LightID;
typedef UniqueID<6> SubSceneID;
typedef UniqueID<7> ViewportID;
typedef UniqueID<8> EntityID;
typedef UniqueID<9> SoundID;

const SubSceneID DefaultSubSceneID = SubSceneID();

const std::string DEFAULT_MATERIAL_SCHEME = "default";

class Mesh;
class Entity;
class Light;
class Scene;
class SubScene;
class Camera;
class ShaderProgram;
class Viewport;
class Frustum;
class WindowBase;
class Material;
class Pipeline;
class Partitioner;
class Sound;

}

#endif // TYPES_H_INCLUDED
