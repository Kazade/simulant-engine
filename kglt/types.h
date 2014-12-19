#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <iostream>
#include <memory>

#include <lua.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include "colour.h"

#include <kazmath/vec3.h>
#include <kazmath/vec2.h>
#include <kazmath/vec4.h>
#include <kazmath/quaternion.h>
#include <kazmath/mat4.h>
#include <kazmath/mat3.h>
#include <kazmath/utility.h>
#include <kazmath/aabb.h>

#include "generic/protected_ptr.h"
#include "generic/auto_weakptr.h"
#include "generic/unique_id.h"
#include "kazbase/unicode.h"

namespace kglt {

struct Vec3;

struct Quaternion : public kmQuaternion {
    Quaternion() {
        kmQuaternionIdentity(this);
    }

    Quaternion(float x, float y, float z, float w) {
        kmQuaternionFill(this, x, y, z, w);
    }

    Quaternion operator-(const Quaternion& quat) const {
        Quaternion result;
        kmQuaternionSubtract(&result, this, &quat);
        return result;
    }

    Quaternion operator*(const Quaternion& quat) const {
        Quaternion result;
        kmQuaternionMultiply(&result, this, &quat);
        return result;
    }

    void normalize() {
        kmQuaternionNormalize(this, this);
    }

    const Quaternion normalized() {
        Quaternion result;
        kmQuaternionNormalize(&result, this);
        return result;
    }

    const Quaternion& inverse() {
        kmQuaternionInverse(this, this);
        return *this;
    }

    bool operator==(const Quaternion& rhs) const {
        return kmQuaternionAreEqual(this, &rhs);
    }

    bool operator!=(const Quaternion& rhs) const {
        return !(*this == rhs);
    }

    Quaternion slerp(const Quaternion& rhs, float t) {
        Quaternion result;
        kmQuaternionSlerp(&result, this, &rhs, t);
        return result;
    }

    static Quaternion look_rotation(const Vec3& direction, const Vec3& up);
};

struct Mat4 : public kmMat4 {
    Mat4() {
        kmMat4Identity(this);
    }

    Mat4 operator*(const Mat4& rhs) const {
        Mat4 result;
        kmMat4Multiply(&result, this, &rhs);
        return result;
    }
};

struct Mat3 : public kmMat3 {};

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

    Vec2 rotated_by(float degrees) const {
        Vec2 result, zero;
        kmVec2RotateBy(&result, this, degrees, &zero);
        return result;
    }

    float length() const {
        return kmVec2Length(this);
    }

    void normalize() {
        kmVec2Normalize(this, this);
    }

    Vec2 limit(float l) {
        if(length() > l) {
            normalize();
            kmVec2Scale(this, this, l);
        }

        return *this;
    }

    bool operator==(const Vec2& rhs) const {
        return kmVec2AreEqual(this, &rhs);
    }

    bool operator!=(const Vec2& rhs) const {
        return !(*this == rhs);
    }

    Vec2 operator*(float rhs) const {
        Vec2 result;
        kmVec2Scale(&result, this, rhs);
        return result;
    }

    Vec2& operator*=(float rhs) {
        kmVec2Scale(this, this, rhs);
        return *this;
    }

    Vec2& operator+=(const Vec2& rhs) {
        kmVec2Add(this, this, &rhs);
        return *this;
    }

    Vec2 operator+(const Vec2& rhs) const {
        return Vec2(x + rhs.x, y + rhs.y);
    }

    unicode to_string() const {
        return _u("({0},{1})").format(x, y);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vec2& vec);
};

struct Degrees;

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

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vec3& operator+=(const Vec3& rhs) {
        kmVec3Add(this, this, &rhs);
        return *this;
    }

    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3 operator*(float rhs) const {
        Vec3 result;
        kmVec3Scale(&result, this, rhs);
        return result;
    }

    Vec3& operator*=(float rhs) {
        kmVec3Scale(this, this, rhs);
        return *this;
    }

    Vec3& operator/=(float rhs) {
        kmVec3Scale(this, this, 1.0 / rhs);
        return *this;
    }

    Vec3 operator/(float rhs) const {
        Vec3 result;
        kmVec3Scale(&result, this, 1.0 / rhs);
        return result;
    }

    bool operator==(const Vec3& rhs) const {
        return kmVec3AreEqual(this, &rhs);
    }

    bool operator!=(const Vec3& rhs) const {
        return !(*this == rhs);
    }

    void set(float x, float y, float z) {
        kmVec3Fill(this, x, y, z);
    }

    float length() const {        
        return kmVec3Length(this);
    }

    float length_squared() const {
        return kmVec3LengthSq(this);
    }

    const kglt::Vec3 normalized() {
        kglt::Vec3 result;
        kmVec3Normalize(&result, this);
        return result;
    }

    void normalize() {
        kmVec3Normalize(this, this);
    }

    Vec3 rotated_by(const Quaternion& q) {
        Vec3 result;
        kmQuaternionMultiplyVec3(&result, &q, this);
        return result;
    }

    float dot(const kglt::Vec3& rhs) const {
        return kmVec3Dot(this, &rhs);
    }

    kglt::Vec3 cross(const kglt::Vec3& rhs) const {
        kglt::Vec3 result;
        kmVec3Cross(&result, this, &rhs);
        return result;
    }

    kglt::Vec3 limit(float l) {
        if(length() > l) {
            normalize();
            kmVec3Scale(this, this, l);
        }

        return *this;
    }

    static float distance(const kglt::Vec3& lhs, const kglt::Vec3& rhs) {
        return (rhs - lhs).length();
    }


    //Neccesary for OpenSteer
    inline Vec3 parallel_component(const Vec3& unit_basis) const {
        const float projection = this->dot(unit_basis);
        return unit_basis * projection;
    }

    // return component of vector perpendicular to a unit basis vector
    // (IMPORTANT NOTE: assumes "basis" has unit magnitude (length==1))

    inline Vec3 perpendicular_component (const Vec3& unit_basis) const {
        return (*this) - parallel_component(unit_basis);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vec3& vec);

    unicode to_string() const {
        return _u("({0},{1},{2})").format(x, y, z);
    }

    Vec3 perpendicular() const;
    Vec3 random_deviant(const Degrees& angle, const Vec3 up=Vec3()) const;
};


struct AABB : public kmAABB {
    AABB() {
        kmVec3Zero(&this->min);
        kmVec3Zero(&this->max);
    }

    const float width() const {
        return fabs(max.x - min.x);
    }

    const float height() const {
        return fabs(max.y - min.y);
    }

    const float depth() const  {
        return fabs(max.z - min.z);
    }
};

std::ostream& operator<<(std::ostream& stream, const Vec2& vec);
std::ostream& operator<<(std::ostream& stream, const Vec3& vec);
std::ostream& operator<<(std::ostream& stream, const Quaternion& quat);

kglt::Vec2 operator*(float lhs, const kglt::Vec2& rhs);

kglt::Vec3 operator*(float lhs, const kglt::Vec3& rhs);
kglt::Vec3 operator/(float lhs, const kglt::Vec3& rhs);
kglt::Vec3 operator-(const kglt::Vec3& vec);
kglt::Quaternion operator-(const kglt::Quaternion& q);

struct Radians;
struct Degrees {
    explicit Degrees(float value):
        value_(value) {}

    Degrees(const Radians& rhs);

    float value_;

    Degrees operator-() const {
        Degrees ret = *this;
        ret.value_ = -ret.value_;
        return ret;
    }
};

struct Radians {
    explicit Radians(float value):
        value_(value) {}

    Radians(const Degrees& rhs);

    float value_;
};

Radians to_radians(const Degrees& degrees);
Degrees to_degrees(const Radians& radians);

const float PI = kmPI;

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
    RENDER_PRIORITY_ABSOLUTE_BACKGROUND = -150,
    RENDER_PRIORITY_BACKGROUND = -100,
    RENDER_PRIORITY_DISTANT = -50,
    RENDER_PRIORITY_MAIN = 0,
    RENDER_PRIORITY_NEAR = 50,
    RENDER_PRIORITY_FOREGROUND = 100,
    RENDER_PRIORITY_ABSOLUTE_FOREGROUND = 150
};

const std::vector<RenderPriority> RENDER_PRIORITIES = {
    RENDER_PRIORITY_BACKGROUND,
    RENDER_PRIORITY_DISTANT,
    RENDER_PRIORITY_MAIN,
    RENDER_PRIORITY_NEAR,
    RENDER_PRIORITY_FOREGROUND
};

enum LoggingLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
};

enum PolygonMode {
    POLYGON_MODE_FILL,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT
};

enum ShaderType {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_MAX
};

enum TextureFlag {
    TEXTURE_OPTION_CLAMP_TO_EDGE = 1,
    TEXTURE_OPTION_FLIP_VERTICALLY = 2,
    TEXTURE_OPTION_DISABLE_MIPMAPS = 4,
    TEXTURE_OPTION_NEAREST_FILTER = 8
};

enum VirtualDPadDirections {
    VIRTUAL_DPAD_DIRECTIONS_TWO,
    VIRTUAL_DPAD_DIRECTIONS_FOUR,
    VIRTUAL_DPAD_DIRECTIONS_EIGHT,
    VIRTUAL_DPAD_DIRECTIONS_ANALOG
};

typedef uint32_t TextureFlags;

typedef UniqueID<0> MeshID;
typedef UniqueID<1> TextureID;
typedef UniqueID<2> CameraID;
typedef UniqueID<4> MaterialID;
typedef UniqueID<5> LightID;
typedef UniqueID<6> StageID;
typedef UniqueID<7> ViewportID;
typedef UniqueID<8> ActorID;
typedef UniqueID<9> SoundID;
typedef UniqueID<10> PipelineID;
typedef UniqueID<11> UIStageID;
typedef UniqueID<12> SpriteID;
typedef UniqueID<13> BackgroundID;
typedef UniqueID<14> ParticleSystemID;

const StageID DefaultStageID = StageID();

const std::string DEFAULT_MATERIAL_SCHEME = "default";

typedef uint16_t SubMeshIndex;

class Mesh;
typedef ProtectedPtr<Mesh> MeshPtr;

class Material;
typedef std::weak_ptr<Material> MaterialRef;
typedef std::shared_ptr<Material> MaterialPtr;

class Texture;
typedef std::weak_ptr<Texture> TextureRef;
typedef std::shared_ptr<Texture> TexturePtr;

class Sound;
typedef std::weak_ptr<Sound> SoundRef;
typedef std::shared_ptr<Sound> SoundPtr;

class Actor;
typedef ProtectedPtr<Actor> ActorPtr;

class ParticleSystem;
typedef ProtectedPtr<ParticleSystem> ParticleSystemPtr;

class Sprite;
typedef ProtectedPtr<Sprite> SpritePtr;

class Light;
typedef std::shared_ptr<Light> LightPtr;

class Camera;
class CameraProxy;

typedef AutoWeakPtr<Camera> CameraPtr;

class UIStage;
typedef AutoWeakPtr<UIStage> UIStagePtr;

class Viewport;
typedef AutoWeakPtr<Viewport> ViewportPtr;

class Background;
typedef AutoWeakPtr<Background> BackgroundPtr;

class Stage;
typedef AutoWeakPtr<Stage> StagePtr;

class ResourceManager;
typedef AutoWeakPtr<ResourceManager> ResourceManagerPtr;

class PhysicsEngine;
typedef AutoWeakPtr<PhysicsEngine> PhysicsEnginePtr;

class RenderSequence;
typedef AutoWeakPtr<RenderSequence> RenderSequencePtr;

class Pipeline;
typedef AutoWeakPtr<Pipeline> PipelinePtr;

class Frustum;
class WindowBase;
class Partitioner;

class GPUProgram;
typedef std::shared_ptr<GPUProgram> GPUProgramPtr;

namespace physics {
    class ODEEngine;
}

typedef physics::ODEEngine DefaultPhysicsEngine;

}

namespace std {

//Until C++14 :/
template<typename T, typename ...Args>
::std::unique_ptr<T> make_unique( Args&& ...args )
{
    return ::std::unique_ptr<T>( new T( ::std::forward<Args>(args)... ) );
}

}


#include "physics/types.h"

#endif // TYPES_H_INCLUDED
