#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cstdint>

namespace GL {

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

typedef uint32_t MeshID;
typedef uint32_t TextureID;
typedef uint32_t CameraID;
typedef uint32_t ShaderID;

const TextureID NullTextureID = 0;
const ShaderID NullShaderID = 0;
const CameraID DefaultCameraID = 0;


}

#endif // TYPES_H_INCLUDED
