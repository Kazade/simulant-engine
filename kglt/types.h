#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cstdint>
#include <string>
#include "colour.h"

namespace kglt {

struct Vec3 {
    float x, y, z;

    Vec3():
        x(0.0f), y(0.0f), z(0.0f) {}
};

struct Vec2 {
    float x, y;
    Vec2():
        x(0.0f), y(0.0f) {}
};


//FIXME: Should be something like UniqueID<0>, UniqueID<1> or something so that
//IDs can't be incorrectly passed
typedef uint32_t MeshID;
typedef uint32_t TextureID;
typedef uint32_t CameraID;
typedef uint32_t ShaderID;
typedef uint32_t SpriteID;
typedef uint32_t FontID;
typedef uint32_t TextID;
typedef uint32_t OverlayID;
typedef uint32_t MaterialID;

const CameraID DefaultCameraID = 0;

const std::string DEFAULT_MATERIAL_SCHEME = "scheme";

class Mesh;

}

#endif // TYPES_H_INCLUDED
