#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <iostream>
#include <memory>
#include <tuple>
#include <cstdint>
#include <string>
#include <vector>
#include "colour.h"

#include "kazmath/vec2.h"
#include "kazmath/vec3.h"
#include "kazmath/vec4.h"
#include "kazmath/ray3.h"
#include "kazmath/mat4.h"
#include "kazmath/mat3.h"
#include "kazmath/aabb3.h"
#include "kazmath/quaternion.h"
#include "kazmath/plane.h"

#include "generic/manager.h"
#include "generic/auto_weakptr.h"
#include "generic/unique_id.h"
#include "kazbase/unicode.h"

#define DEFINE_SIGNAL(prototype, name) \
    public: \
        prototype& name() { return name##_; } \
    private: \
        prototype name##_;


namespace kglt {

struct Vec3;
struct Mat3;

struct Quaternion : public kmQuaternion {
    Quaternion(const kmQuaternion& other) {
        kmQuaternionAssign(this, &other);
    }

    Quaternion() {
        kmQuaternionIdentity(this);
    }

    Quaternion(const Mat3& rot_matrix);

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

    void inverse() {
        kmQuaternionInverse(this, this);
    }

    const Quaternion inversed() const {
        Quaternion result(*this);
        result.inverse();
        return result;
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

    const float pitch() const {
        return kmQuaternionGetPitch(this);
    }

    const float yaw() const {
        return kmQuaternionGetYaw(this);
    }

    const float roll() const {
        return kmQuaternionGetRoll(this);
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

    void extract_rotation_and_translation(Quaternion& rotation, Vec3& translation);
    static Mat4 from_lookat(const Vec3& eye, const Vec3& target, const Vec3& up);

};

struct Mat3 : public kmMat3 {
    static Mat3 from_rotation_x(float pitch);
    static Mat3 from_rotation_y(float yaw);
    static Mat3 from_rotation_z(float roll);

    Mat3() {
        kmMat3Identity(this);
    }

    Mat3(const float* data) {
        for(uint32_t i = 0; i < 9; ++i) {
            this->mat[i] = data[i];
        }
    }
};

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

    bool operator==(const Vec4& rhs) const {
        return kmVec4AreEqual(this, &rhs);
    }

    bool operator!=(const Vec4& rhs) const {
        return !(*this == rhs);
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

    Vec2 normalized() const {
        Vec2 ret;
        kmVec2Normalize(&ret, this);
        return ret;
    }

    Vec2 limit(float l) {
        if(length() > l) {
            normalize();
            kmVec2Scale(this, this, l);
        }

        return *this;
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

    Vec2& operator-=(const Vec2& rhs) {
        kmVec2Subtract(this, this, &rhs);
        return *this;
    }

    Vec2 operator+(const Vec2& rhs) const {
        return Vec2(x + rhs.x, y + rhs.y);
    }

    Vec2& operator/=(float rhs) {
        kmVec2Scale(this, this, 1.0 / rhs);
        return *this;
    }

    Vec2 operator/(float rhs) const {
        Vec2 result;
        kmVec2Scale(&result, this, 1.0 / rhs);
        return result;
    }

    Vec2 operator-() const {
        return Vec2(-x, -y);
    }

    Vec2 operator-(const Vec2& rhs) const {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    float dot(const Vec2& rhs) const {
        return kmVec2Dot(this, &rhs);
    }

    unicode to_string() const {
        return _u("({0},{1})").format(x, y);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Vec2& vec);
};

bool operator==(const Vec2& lhs, const Vec2& rhs);
bool operator!=(const Vec2& lhs, const Vec2& rhs);

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

    Vec2 xy() const {
        return kglt::Vec2(x, y);
    }

    Vec2 yx() const {
        return kglt::Vec2(y, x);
    }

    Vec3 zyx() const {
        return kglt::Vec3(z, y, x);
    }

    Vec2 xz() const {
        return kglt::Vec2(x, z);
    }

    Vec2 zx() const {
        return kglt::Vec2(z, x);
    }

    Vec3 operator+(const kmVec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vec3& operator+=(const kmVec3& rhs) {
        kmVec3Add(this, this, &rhs);
        return *this;
    }

    Vec3 operator-(const kmVec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3 operator*(float rhs) const {
        Vec3 result;
        kmVec3Scale(&result, this, rhs);
        return result;
    }

    Vec3 operator*(const Quaternion& rhs) const {
        Vec3 ret;
        kmQuaternionMultiplyVec3(&ret, &rhs, this);
        return ret;
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

    Vec3 lerp(const Vec3& end, const float t) {
        Vec3 ret;
        kmVec3Lerp(&ret, this, &end, t);
        return ret;
    }

    Vec3 rotated_by(const Quaternion& q) const {
        Vec3 result;
        kmQuaternionMultiplyVec3(&result, &q, this);
        return result;
    }

    Vec3 rotated_by(const Mat3& rot) const {
        Vec3 result;
        kmVec3MultiplyMat3(&result, this, &rot);
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

struct Plane : public kmPlane {
    Plane(const Vec3& N, float D) {
        kmPlaneFromNormalAndDistance(this, &N, D);
    }

    Vec3 project(const Vec3& v) {
        Vec3 ret;
        kmVec3ProjectOnToPlane(&ret, &v, this);
        return ret;
    }
};

struct Ray : public kmRay3 {

};

struct AABB : public kmAABB3 {
    AABB() {
        kmVec3Zero(&this->min);
        kmVec3Zero(&this->max);
    }

    AABB(const Vec3& centre, float width) {
        kmAABB3Initialize(this, &centre, width, width, width);
    }

    AABB(const Vec3& centre, float xsize, float ysize, float zsize) {
        kmAABB3Initialize(this, &centre, xsize, ysize, zsize);
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

    const float max_dimension() const {
        return std::max(width(), std::max(height(), depth()));
    }

    bool intersects(const AABB& other) const {
        return kmAABB3IntersectsAABB(this, &other);
    }

    Vec3 centre() const {
        return Vec3(min) + ((Vec3(max) - Vec3(min)) * 0.5f);
    }

    const bool has_zero_area() const {
        /*
         * Returns True if the AABB has two or more zero dimensions
         */
        bool empty_x = kmAlmostEqual(0.0, width());
        bool empty_y = kmAlmostEqual(0.0, height());
        bool empty_z = kmAlmostEqual(0.0, depth());

        return (empty_x && empty_y) || (empty_x && empty_z) || (empty_y && empty_z);
    }
};

std::ostream& operator<<(std::ostream& stream, const Vec2& vec);
std::ostream& operator<<(std::ostream& stream, const Vec3& vec);
std::ostream& operator<<(std::ostream& stream, const Vec4& vec);
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

enum AvailablePartitioner {
    PARTITIONER_NULL,
    PARTITIONER_OCTREE
};

enum LightType {
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_SPOT_LIGHT
};

enum AspectRatio {
    ASPECT_RATIO_CUSTOM,
    ASPECT_RATIO_4_BY_3,
    ASPECT_RATIO_16_BY_9,
    ASPECT_RATIO_16_BY_10
};

typedef float Ratio; //FIXME: Custom type to restrict between 0 and 1

enum ProjectionType {
    PROJECTION_TYPE_PERSPECTIVE,
    PROJECTION_TYPE_ORTHOGRAPHIC
};

enum BufferClearFlag {
    BUFFER_CLEAR_COLOUR_BUFFER = 0x1,
    BUFFER_CLEAR_DEPTH_BUFFER = 0x2,
    BUFFER_CLEAR_STENCIL_BUFFER = 0x4,
    BUFFER_CLEAR_ALL = BUFFER_CLEAR_COLOUR_BUFFER | BUFFER_CLEAR_DEPTH_BUFFER | BUFFER_CLEAR_STENCIL_BUFFER
};

typedef int32_t RenderPriority;
const RenderPriority RENDER_PRIORITY_ABSOLUTE_BACKGROUND = -250;
const RenderPriority RENDER_PRIORITY_BACKGROUND = -100;
const RenderPriority RENDER_PRIORITY_DISTANT = -50;
const RenderPriority RENDER_PRIORITY_MAIN = 0;
const RenderPriority RENDER_PRIORITY_NEAR = 50;
const RenderPriority RENDER_PRIORITY_FOREGROUND = 100;
const RenderPriority RENDER_PRIORITY_ABSOLUTE_FOREGROUND = 250;
const RenderPriority RENDER_PRIORITY_MAX = RENDER_PRIORITY_ABSOLUTE_FOREGROUND + 1;

const std::vector<RenderPriority> RENDER_PRIORITIES = {
    RENDER_PRIORITY_ABSOLUTE_BACKGROUND,
    RENDER_PRIORITY_BACKGROUND,
    RENDER_PRIORITY_DISTANT,
    RENDER_PRIORITY_MAIN,
    RENDER_PRIORITY_NEAR,
    RENDER_PRIORITY_FOREGROUND,
    RENDER_PRIORITY_ABSOLUTE_FOREGROUND
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

enum RenderableCullingMode {
    RENDERABLE_CULLING_MODE_NEVER,
    RENDERABLE_CULLING_MODE_PARTITIONER
};

enum CullMode {
    CULL_MODE_NONE,
    CULL_MODE_BACK_FACE,
    CULL_MODE_FRONT_FACE,
    CULL_MODE_FRONT_AND_BACK_FACE
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

typedef UniqueID<0> MeshID;
typedef UniqueID<1> SubMeshID;
typedef UniqueID<2> TextureID;
typedef UniqueID<3> CameraID;
typedef UniqueID<4> MaterialID;
typedef UniqueID<5> LightID;
typedef UniqueID<6> StageID;
typedef UniqueID<8> ActorID;
typedef UniqueID<9> GeomID;
typedef UniqueID<10> SoundID;
typedef UniqueID<11> PipelineID;
typedef UniqueID<12> OverlayID;
typedef UniqueID<13> SpriteID;
typedef UniqueID<14> BackgroundID;
typedef UniqueID<15> ParticleSystemID;
typedef UniqueID<16> SkyboxID;
typedef UniqueID<17> ShaderID;

const StageID DefaultStageID = StageID();

const std::string DEFAULT_MATERIAL_SCHEME = "default";

class Mesh;
typedef std::weak_ptr<Mesh> MeshRef;
typedef std::shared_ptr<Mesh> MeshPtr;

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
typedef Actor* ActorPtr;

class Geom;
typedef Geom* GeomPtr;

class ParticleSystem;
typedef ParticleSystem* ParticleSystemPtr;

class Sprite;
typedef Sprite* SpritePtr;

class Light;
typedef Light* LightPtr;

class Camera;
class CameraProxy;

typedef Camera* CameraPtr;
typedef CameraProxy* CameraProxyPtr;

class Overlay;
typedef Overlay* OverlayPtr;

class Viewport;

class Background;
typedef Background* BackgroundPtr;

class Stage;
class WindowBase;
typedef generic::TemplatedManager<Stage, StageID> BaseStageManager;
typedef Stage* StagePtr;

class ResourceManager;
typedef AutoWeakPtr<ResourceManager> ResourceManagerPtr;

class RenderSequence;
typedef AutoWeakPtr<RenderSequence> RenderSequencePtr;

class Pipeline;
typedef AutoWeakPtr<Pipeline> PipelinePtr;

class Frustum;
class WindowBase;
class Partitioner;

class GPUProgram;
typedef std::shared_ptr<GPUProgram> GPUProgramPtr;

}

/* Hash functions for kglt types */
namespace std {
    template <> struct hash<kglt::MeshArrangement> {
        size_t operator() (kglt::MeshArrangement t) { return size_t(t); }
    };
}



// Generic hash for tuples by Leo Goodstadt
// http://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace std{
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v)
        {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
          static void apply(size_t& seed, Tuple const& tuple)
          {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, std::get<Index>(tuple));
          }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0>
        {
          static void apply(size_t& seed, Tuple const& tuple)
          {
            hash_combine(seed, std::get<0>(tuple));
          }
        };
    }

    template <typename ... TT>
    struct hash<std::tuple<TT...>>
    {
        size_t
        operator()(std::tuple<TT...> const& tt) const
        {
            size_t seed = 0;
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
            return seed;
        }

    };
}


#endif // TYPES_H_INCLUDED
