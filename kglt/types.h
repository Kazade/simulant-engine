#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cstdint>
#include <string>
#include "colour.h"

#include "kazmath/vec3.h"
#include "kazmath/vec2.h"
#include "kazmath/vec4.h"

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


//FIXME: Should be something like UniqueID<0>, UniqueID<1> or something so that
//IDs can't be incorrectly passed
typedef uint32_t MeshID;
typedef uint32_t TextureID;
typedef uint32_t CameraID;
typedef uint32_t ShaderID;
typedef uint32_t FontID;
typedef uint32_t TextID;
typedef uint32_t OverlayID;
typedef uint32_t MaterialID;
typedef uint32_t LightID;

const CameraID DefaultCameraID = 0;

const std::string DEFAULT_MATERIAL_SCHEME = "scheme";

class Mesh;
class Light;
class Scene;
class Camera;

}

#endif // TYPES_H_INCLUDED
