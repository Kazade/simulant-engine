/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <iostream>
#include <memory>
#include <tuple>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "color.h"
#include "generic/optional.h"

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/quaternion.h"
#include "math/degrees.h"
#include "math/radians.h"
#include "math/mat3.h"
#include "math/mat4.h"
#include "math/aabb.h"
#include "math/plane.h"
#include "math/ray.h"
#include "math/operators.h"

#include "generic/object_manager.h"
#include "core/stage_node_id.h"
#include "core/asset_id.h"

#include "utils/unicode.h"
#include "material_constants.h"

namespace smlt {

class Seconds {
public:
    Seconds():
        value_(0) {}

    explicit Seconds(float t):
        value_(t) {}

    Seconds operator+(const Seconds& rhs) const {
        return Seconds(value_ + rhs.value_);
    }

    Seconds& operator+=(const Seconds& rhs) {
        value_ += rhs.value_;
        return *this;
    }

    bool operator>(float rhs) const {
        return value_ > rhs;
    }

    float to_float() const {
        return value_;
    }

private:
    float value_;
};

typedef std::vector<int> IntArray;
typedef std::vector<float> FloatArray;
typedef std::vector<bool> BoolArray;

enum IndexType {
    INDEX_TYPE_8_BIT,
    INDEX_TYPE_16_BIT,
    INDEX_TYPE_32_BIT
};

enum MeshArrangement {
    MESH_ARRANGEMENT_TRIANGLES,
    MESH_ARRANGEMENT_TRIANGLE_FAN,
    MESH_ARRANGEMENT_TRIANGLE_STRIP,
    MESH_ARRANGEMENT_QUADS,
    MESH_ARRANGEMENT_LINES,
    MESH_ARRANGEMENT_LINE_STRIP
};

enum AvailablePartitioner {
    PARTITIONER_NULL,
    PARTITIONER_FRUSTUM,
    PARTITIONER_HASH
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
    BUFFER_CLEAR_COLOR_BUFFER = 0x1,
    BUFFER_CLEAR_DEPTH_BUFFER = 0x2,
    BUFFER_CLEAR_STENCIL_BUFFER = 0x4,
    BUFFER_CLEAR_ALL = BUFFER_CLEAR_COLOR_BUFFER | BUFFER_CLEAR_DEPTH_BUFFER | BUFFER_CLEAR_STENCIL_BUFFER
};

typedef int32_t RenderPriority;
const RenderPriority RENDER_PRIORITY_MIN = -25;
const RenderPriority RENDER_PRIORITY_ABSOLUTE_BACKGROUND = -25;
const RenderPriority RENDER_PRIORITY_BACKGROUND = -10;
const RenderPriority RENDER_PRIORITY_DISTANT = -5;
const RenderPriority RENDER_PRIORITY_MAIN = 0;
const RenderPriority RENDER_PRIORITY_NEAR = 5;
const RenderPriority RENDER_PRIORITY_FOREGROUND = 10;
const RenderPriority RENDER_PRIORITY_ABSOLUTE_FOREGROUND = 25;
const RenderPriority RENDER_PRIORITY_MAX = RENDER_PRIORITY_ABSOLUTE_FOREGROUND + 1;

extern const std::vector<RenderPriority> RENDER_PRIORITIES;

enum ShaderType {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_MAX
};

enum VirtualGamepadConfig {
    VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS,
    VIRTUAL_GAMEPAD_CONFIG_HAT_AND_BUTTON
};

#define DEFAULT_MATERIAL_SCHEME "default"

class Mesh;
typedef std::weak_ptr<Mesh> MeshRef;
typedef std::shared_ptr<Mesh> MeshPtr;

class SubMesh;
typedef SubMesh* SubMeshPtr;

class Material;
typedef std::weak_ptr<Material> MaterialRef;
typedef std::shared_ptr<Material> MaterialPtr;

class Texture;
typedef std::weak_ptr<Texture> TextureRef;
typedef std::shared_ptr<Texture> TexturePtr;

class ParticleScript;
typedef std::shared_ptr<ParticleScript> ParticleScriptPtr;

class Binary;
typedef std::shared_ptr<Binary> BinaryPtr;

class Sound;
typedef std::weak_ptr<Sound> SoundRef;
typedef std::shared_ptr<Sound> SoundPtr;

class Font;
typedef std::weak_ptr<Font> FontRef;
typedef std::shared_ptr<Font> FontPtr;

class Actor;
typedef Actor* ActorPtr;

class Geom;
typedef Geom* GeomPtr;

class MeshInstancer;
typedef MeshInstancer* MeshInstancerPtr;

class ParticleSystem;
typedef ParticleSystem* ParticleSystemPtr;

class Sprite;
typedef Sprite* SpritePtr;

class Light;
typedef Light* LightPtr;

class Camera;
class CameraProxy;

typedef Camera* CameraPtr;

class Viewport;
class Window;

class Stage;
typedef Stage* StagePtr;

namespace ui {

class Widget;
class ProgressBar;
class Button;
class Label;
class TextEntry;

typedef Widget* WidgetPtr;
}

class AssetManager;
typedef std::shared_ptr<AssetManager> ResourceManagerPtr;

class Compositor;
typedef std::shared_ptr<Compositor> CompositorPtr;

class Layer;
typedef Layer* LayerPtr;

class Frustum;
class Window;
class Partitioner;

class GPUProgram;
typedef std::shared_ptr<GPUProgram> GPUProgramPtr;

class Skybox;
typedef Skybox* SkyboxPtr;

typedef uint32_t GPUProgramID;


#ifdef __WIN32__
typedef unsigned long DWORD;
typedef DWORD ProcessID;
#else
typedef uint32_t ProcessID;
#endif

// Attributes should be aligned at 4 byte boundaries
// according to this
// https://developer.apple.com/library/content/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/TechniquesforWorkingwithVertexData/TechniquesforWorkingwithVertexData.html
static const uint16_t BUFFER_ATTRIBUTE_ALIGNMENT = 4;

// According to Nvidia, stride should be 16 byte aligned
static const uint16_t BUFFER_STRIDE_ALIGNMENT = 16;

/* Most platforms work better when the data is aligned to 4 byte boundaries */
constexpr uint16_t round_to_bytes(uint16_t stride, uint16_t bytes) {
    return ((stride % bytes) == 0) ? stride : stride + bytes - (stride % bytes);
}

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


#define DEFINE_ENUM_HASH(enum_name) \
    template <> struct hash<enum_name> { \
        size_t operator() (const enum_name& t) const { return std::hash<size_t>()((size_t) t); } \
    }

    /* Hash functions for smlt types */
    DEFINE_ENUM_HASH(smlt::MeshArrangement);
    DEFINE_ENUM_HASH(smlt::IndexType);

}

#endif // TYPES_H_INCLUDED
