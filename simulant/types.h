/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
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

#include "colour.h"
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

#include "generic/manager.h"
#include "generic/auto_weakptr.h"
#include "generic/unique_id.h"
#include "generic/default_init_ptr.h"

#include "utils/unicode.h"
#include "material_constants.h"

#define DEFINE_SIGNAL(prototype, name) \
    public: \
        prototype& name() { return name##_; } \
    private: \
        prototype name##_;


namespace smlt {

enum VertexAttribute {
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_4F
};

class VertexSpecification;

/*
 * Allows us to keep the original syntax of VertexSpecification when it was a struct
 * but also ensure that stride and offsets are updated when a value changes
 * C++ really could do with properties built-in :(
 */
class VertexAttributeProperty {
public:
    VertexAttributeProperty(VertexSpecification* spec, VertexAttribute VertexSpecification::* attr);

    operator VertexAttribute() const {
        return spec_->*attr_;
    }

    VertexAttributeProperty& operator=(const VertexAttribute& rhs);

    VertexAttributeProperty(const VertexAttributeProperty& rhs) = delete;
    VertexAttributeProperty& operator=(const VertexAttributeProperty& rhs) = delete;
private:
    VertexSpecification* spec_ = nullptr;
    VertexAttribute VertexSpecification::* attr_ = nullptr;
};


class VertexSpecification {
    VertexAttribute position_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute normal_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord0_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord1_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord2_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord3_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord4_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord5_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord6_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute texcoord7_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute diffuse_attribute_ = VERTEX_ATTRIBUTE_NONE;
    VertexAttribute specular_attribute_ = VERTEX_ATTRIBUTE_NONE;

    uint16_t normal_offset_ = 0;
    uint16_t texcoord0_offset_ = 0;
    uint16_t texcoord1_offset_ = 0;
    uint16_t texcoord2_offset_ = 0;
    uint16_t texcoord3_offset_ = 0;
    uint16_t texcoord4_offset_ = 0;
    uint16_t texcoord5_offset_ = 0;
    uint16_t texcoord6_offset_ = 0;
    uint16_t texcoord7_offset_ = 0;
    uint16_t diffuse_offset_ = 0;
    uint16_t specular_offset_ = 0;

public:
    static const VertexSpecification DEFAULT;
    static const VertexSpecification POSITION_ONLY;
    static const VertexSpecification POSITION_AND_DIFFUSE;

    VertexAttributeProperty position_attribute = {this, &VertexSpecification::position_attribute_};
    VertexAttributeProperty normal_attribute = {this, &VertexSpecification::normal_attribute_};
    VertexAttributeProperty texcoord0_attribute = {this, &VertexSpecification::texcoord0_attribute_};
    VertexAttributeProperty texcoord1_attribute = {this, &VertexSpecification::texcoord1_attribute_};
    VertexAttributeProperty texcoord2_attribute = {this, &VertexSpecification::texcoord2_attribute_};
    VertexAttributeProperty texcoord3_attribute = {this, &VertexSpecification::texcoord3_attribute_};
    VertexAttributeProperty texcoord4_attribute = {this, &VertexSpecification::texcoord4_attribute_};
    VertexAttributeProperty texcoord5_attribute = {this, &VertexSpecification::texcoord5_attribute_};
    VertexAttributeProperty texcoord6_attribute = {this, &VertexSpecification::texcoord6_attribute_};
    VertexAttributeProperty texcoord7_attribute = {this, &VertexSpecification::texcoord7_attribute_};
    VertexAttributeProperty diffuse_attribute = {this, &VertexSpecification::diffuse_attribute_};
    VertexAttributeProperty specular_attribute = {this, &VertexSpecification::specular_attribute_};

    VertexSpecification() = default;
    VertexSpecification(const VertexSpecification&& rhs):
        position_attribute_(rhs.position_attribute_),
        normal_attribute_(rhs.normal_attribute_),
        texcoord0_attribute_(rhs.texcoord0_attribute_),
        texcoord1_attribute_(rhs.texcoord1_attribute_),
        texcoord2_attribute_(rhs.texcoord2_attribute_),
        texcoord3_attribute_(rhs.texcoord3_attribute_),
        texcoord4_attribute_(rhs.texcoord4_attribute_),
        texcoord5_attribute_(rhs.texcoord5_attribute_),
        texcoord6_attribute_(rhs.texcoord6_attribute_),
        texcoord7_attribute_(rhs.texcoord7_attribute_),
        diffuse_attribute_(rhs.diffuse_attribute_),
        specular_attribute_(rhs.specular_attribute_),
        position_attribute(this, &VertexSpecification::position_attribute_),
        normal_attribute(this, &VertexSpecification::normal_attribute_),
        texcoord0_attribute(this, &VertexSpecification::texcoord0_attribute_),
        texcoord1_attribute(this, &VertexSpecification::texcoord1_attribute_),
        texcoord2_attribute(this, &VertexSpecification::texcoord2_attribute_),
        texcoord3_attribute(this, &VertexSpecification::texcoord3_attribute_),
        texcoord4_attribute(this, &VertexSpecification::texcoord4_attribute_),
        texcoord5_attribute(this, &VertexSpecification::texcoord5_attribute_),
        texcoord6_attribute(this, &VertexSpecification::texcoord6_attribute_),
        texcoord7_attribute(this, &VertexSpecification::texcoord7_attribute_),
        diffuse_attribute(this, &VertexSpecification::diffuse_attribute_),
        specular_attribute(this, &VertexSpecification::specular_attribute_) {

        recalc_stride_and_offsets();
    }

    VertexSpecification(const VertexSpecification& rhs):
        position_attribute_(rhs.position_attribute_),
        normal_attribute_(rhs.normal_attribute_),
        texcoord0_attribute_(rhs.texcoord0_attribute_),
        texcoord1_attribute_(rhs.texcoord1_attribute_),
        texcoord2_attribute_(rhs.texcoord2_attribute_),
        texcoord3_attribute_(rhs.texcoord3_attribute_),
        texcoord4_attribute_(rhs.texcoord4_attribute_),
        texcoord5_attribute_(rhs.texcoord5_attribute_),
        texcoord6_attribute_(rhs.texcoord6_attribute_),
        texcoord7_attribute_(rhs.texcoord7_attribute_),
        diffuse_attribute_(rhs.diffuse_attribute_),
        specular_attribute_(rhs.specular_attribute_),
        position_attribute(this, &VertexSpecification::position_attribute_),
        normal_attribute(this, &VertexSpecification::normal_attribute_),
        texcoord0_attribute(this, &VertexSpecification::texcoord0_attribute_),
        texcoord1_attribute(this, &VertexSpecification::texcoord1_attribute_),
        texcoord2_attribute(this, &VertexSpecification::texcoord2_attribute_),
        texcoord3_attribute(this, &VertexSpecification::texcoord3_attribute_),
        texcoord4_attribute(this, &VertexSpecification::texcoord4_attribute_),
        texcoord5_attribute(this, &VertexSpecification::texcoord5_attribute_),
        texcoord6_attribute(this, &VertexSpecification::texcoord6_attribute_),
        texcoord7_attribute(this, &VertexSpecification::texcoord7_attribute_),
        diffuse_attribute(this, &VertexSpecification::diffuse_attribute_),
        specular_attribute(this, &VertexSpecification::specular_attribute_) {

        recalc_stride_and_offsets();
    }

    VertexSpecification& operator=(const VertexSpecification& rhs) {
        position_attribute_ = rhs.position_attribute_;
        normal_attribute_ = rhs.normal_attribute_;
        texcoord0_attribute_ = rhs.texcoord0_attribute_;
        texcoord1_attribute_ = rhs.texcoord1_attribute_;
        texcoord2_attribute_ = rhs.texcoord2_attribute_;
        texcoord3_attribute_ = rhs.texcoord3_attribute_;
        texcoord4_attribute_ = rhs.texcoord4_attribute_;
        texcoord5_attribute_ = rhs.texcoord5_attribute_;
        texcoord6_attribute_ = rhs.texcoord6_attribute_;
        texcoord7_attribute_ = rhs.texcoord7_attribute_;
        diffuse_attribute_ = rhs.diffuse_attribute_;
        specular_attribute_ = rhs.specular_attribute_;

        recalc_stride_and_offsets();

        return *this;
    }

    VertexSpecification(
        VertexAttribute position,
        VertexAttribute normal = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord0 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord1 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord2 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord3 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord4 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord5 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord6 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute texcoord7 = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute diffuse = VERTEX_ATTRIBUTE_NONE,
        VertexAttribute specular = VERTEX_ATTRIBUTE_NONE
    );

    bool operator==(const VertexSpecification& rhs) const {
        return position_attribute == rhs.position_attribute &&
               normal_attribute == rhs.normal_attribute  &&
                texcoord0_attribute == rhs.texcoord0_attribute &&
                texcoord1_attribute == rhs.texcoord1_attribute &&
                texcoord2_attribute == rhs.texcoord2_attribute &&
                texcoord3_attribute == rhs.texcoord3_attribute &&
                texcoord4_attribute == rhs.texcoord4_attribute &&
                texcoord5_attribute == rhs.texcoord5_attribute &&
                texcoord6_attribute == rhs.texcoord6_attribute &&
                texcoord7_attribute == rhs.texcoord7_attribute &&
                diffuse_attribute == rhs.diffuse_attribute &&
                specular_attribute == rhs.specular_attribute;
    }

    bool operator!=(const VertexSpecification& rhs) const {
        return !(*this == rhs);
    }

    inline uint32_t stride() const { return stride_; }

    bool has_positions() const { return bool(position_attribute_); }
    bool has_normals() const { return bool(normal_attribute_); }

    bool has_texcoordX(uint8_t which) const;

    const VertexAttribute texcoordX_attribute(uint8_t which) const;

    bool has_texcoord0() const { return bool(texcoord0_attribute_); }
    bool has_texcoord1() const { return bool(texcoord1_attribute_); }
    bool has_texcoord2() const { return bool(texcoord2_attribute_); }
    bool has_texcoord3() const { return bool(texcoord3_attribute_); }
    bool has_texcoord4() const { return bool(texcoord4_attribute_); }
    bool has_texcoord5() const { return bool(texcoord5_attribute_); }
    bool has_texcoord6() const { return bool(texcoord6_attribute_); }
    bool has_texcoord7() const { return bool(texcoord7_attribute_); }

    bool has_diffuse() const { return bool(diffuse_attribute_); }
    bool has_specular() const { return bool(specular_attribute_); }

    uint16_t position_offset(bool check=true) const;
    uint16_t normal_offset(bool check=true) const;
    uint16_t texcoord0_offset(bool check=true) const;
    uint16_t texcoord1_offset(bool check=true) const;
    uint16_t texcoord2_offset(bool check=true) const;
    uint16_t texcoord3_offset(bool check=true) const;
    uint16_t texcoord4_offset(bool check=true) const;
    uint16_t texcoord5_offset(bool check=true) const;
    uint16_t texcoord6_offset(bool check=true) const;
    uint16_t texcoord7_offset(bool check=true) const;

    uint16_t texcoordX_offset(uint8_t which, bool check=true) const;

    uint16_t diffuse_offset(bool check=true) const;
    uint16_t specular_offset(bool check=true) const;

private:
    friend class VertexAttributeProperty;

    uint16_t stride_ = 0;

    void recalc_stride_and_offsets();
};

enum IndexType {
    INDEX_TYPE_8_BIT,
    INDEX_TYPE_16_BIT,
    INDEX_TYPE_32_BIT
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
    MESH_ARRANGEMENT_TRIANGLES,
    MESH_ARRANGEMENT_TRIANGLE_FAN,
    MESH_ARRANGEMENT_TRIANGLE_STRIP,
    MESH_ARRANGEMENT_LINES,
    MESH_ARRANGEMENT_LINE_STRIP
};

enum AvailablePartitioner {
    PARTITIONER_NULL,
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

enum ShadeModel {
    SHADE_MODEL_SMOOTH,
    SHADE_MODEL_FLAT
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

enum VirtualGamepadConfig {
    VIRTUAL_GAMEPAD_CONFIG_TWO_BUTTONS,
    VIRTUAL_GAMEPAD_CONFIG_HAT_AND_BUTTON
};

const std::string DEFAULT_MATERIAL_SCHEME = "default";

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

class Sound;
typedef std::weak_ptr<Sound> SoundRef;
typedef std::shared_ptr<Sound> SoundPtr;

class Font;
typedef std::weak_ptr<Font> FontRef;
typedef std::shared_ptr<Font> FontPtr;

class Actor;
typedef default_init_ptr<Actor> ActorPtr;

class Geom;
typedef default_init_ptr<Geom> GeomPtr;

class ParticleSystem;
typedef default_init_ptr<ParticleSystem> ParticleSystemPtr;

class Sprite;
typedef default_init_ptr<Sprite> SpritePtr;

class Light;
typedef default_init_ptr<Light> LightPtr;

class Camera;
class CameraProxy;

typedef default_init_ptr<Camera> CameraPtr;

class Viewport;

class Background;
typedef default_init_ptr<Background> BackgroundPtr;

class Stage;
class Window;
typedef default_init_ptr<Stage> StagePtr;

namespace ui {

class Widget;
class ProgressBar;
class Button;
class Label;

typedef default_init_ptr<Widget> WidgetPtr;

}

class ResourceManager;
typedef AutoWeakPtr<ResourceManager> ResourceManagerPtr;

class RenderSequence;
typedef AutoWeakPtr<RenderSequence> RenderSequencePtr;

class Pipeline;
typedef default_init_ptr<Pipeline> PipelinePtr;

class Frustum;
class Window;
class Partitioner;

class GPUProgram;
typedef std::shared_ptr<GPUProgram> GPUProgramPtr;

class Skybox;
typedef Skybox* SkyboxPtr;

typedef uint32_t IdleConnectionID;

typedef UniqueID<MeshPtr> MeshID;
typedef UniqueID<TexturePtr> TextureID;
typedef UniqueID<FontPtr> FontID;
typedef UniqueID<CameraPtr> CameraID;
typedef UniqueID<MaterialPtr> MaterialID;
typedef UniqueID<LightPtr> LightID;
typedef UniqueID<StagePtr> StageID;
typedef UniqueID<ActorPtr> ActorID;
typedef UniqueID<GeomPtr> GeomID;
typedef UniqueID<SoundPtr> SoundID;
typedef UniqueID<PipelinePtr> PipelineID;
typedef UniqueID<SpritePtr> SpriteID;
typedef UniqueID<BackgroundPtr> BackgroundID;
typedef UniqueID<ParticleSystemPtr> ParticleSystemID;
typedef UniqueID<SkyboxPtr> SkyID;
typedef UniqueID<GPUProgramPtr> GPUProgramID;
typedef UniqueID<ui::WidgetPtr> WidgetID;

typedef generic::TemplatedManager<Stage, StageID> BaseStageManager;

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

constexpr uint16_t vertex_attribute_size(VertexAttribute attr) {
    return round_to_bytes(
       (attr == VERTEX_ATTRIBUTE_2F) ? sizeof(float) * 2 :
       (attr == VERTEX_ATTRIBUTE_3F) ? sizeof(float) * 3 :
       (attr == VERTEX_ATTRIBUTE_4F) ? sizeof(float) * 4 : 0,
        BUFFER_ATTRIBUTE_ALIGNMENT
    );
}

}

/* Hash functions for smlt types */
namespace std {
    template <> struct hash<smlt::MeshArrangement> {
        size_t operator() (smlt::MeshArrangement t) { return size_t(t); }
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
