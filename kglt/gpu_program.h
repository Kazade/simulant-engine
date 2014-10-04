#ifndef GPU_PROGRAM_H
#define GPU_PROGRAM_H

#include <set>
#include <map>
#include <unordered_map>
#include <kazbase/signals.h>

#include "types.h"
#include "generic/managed.h"
#include "utils/gl_thread_check.h"

#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + (bytes))

namespace kglt {

/*
  Automatic uniforms that are set by the renderer
*/
enum ShaderAvailableAuto {
    SP_AUTO_MODELVIEW_PROJECTION_MATRIX,
    SP_AUTO_VIEW_MATRIX,
    SP_AUTO_MODELVIEW_MATRIX,
    SP_AUTO_PROJECTION_MATRIX,
    SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX,
    SP_AUTO_MATERIAL_DIFFUSE,
    SP_AUTO_MATERIAL_SPECULAR,
    SP_AUTO_MATERIAL_AMBIENT,
    SP_AUTO_MATERIAL_SHININESS,
    SP_AUTO_MATERIAL_TEX_MATRIX0,
    SP_AUTO_MATERIAL_TEX_MATRIX1,
    SP_AUTO_MATERIAL_TEX_MATRIX2,
    SP_AUTO_MATERIAL_TEX_MATRIX3,
    SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS,
    SP_AUTO_LIGHT_GLOBAL_AMBIENT,
    SP_AUTO_LIGHT_POSITION,
    SP_AUTO_LIGHT_DIRECTION,
    SP_AUTO_LIGHT_DIFFUSE,
    SP_AUTO_LIGHT_SPECULAR,
    SP_AUTO_LIGHT_AMBIENT,
    SP_AUTO_LIGHT_CONSTANT_ATTENUATION,
    SP_AUTO_LIGHT_LINEAR_ATTENUATION,
    SP_AUTO_LIGHT_QUADRATIC_ATTENUATION

    //TODO: cameras(?)
};

const std::set<ShaderAvailableAuto> SHADER_AVAILABLE_AUTOS = {
    SP_AUTO_MODELVIEW_PROJECTION_MATRIX,
    SP_AUTO_MODELVIEW_MATRIX,
    SP_AUTO_PROJECTION_MATRIX,
    SP_AUTO_MATERIAL_DIFFUSE,
    SP_AUTO_MATERIAL_SPECULAR,
    SP_AUTO_MATERIAL_AMBIENT,
    SP_AUTO_MATERIAL_SHININESS,
    SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS
};

enum ShaderAvailableAttributes {
    SP_ATTR_VERTEX_POSITION,
    SP_ATTR_VERTEX_DIFFUSE,
    SP_ATTR_VERTEX_NORMAL,
    SP_ATTR_VERTEX_TEXCOORD0,
    SP_ATTR_VERTEX_TEXCOORD1,
    SP_ATTR_VERTEX_TEXCOORD2,
    SP_ATTR_VERTEX_TEXCOORD3,
    SP_ATTR_VERTEX_TEXCOORD4,
    SP_ATTR_VERTEX_TEXCOORD5,
    SP_ATTR_VERTEX_TEXCOORD6,
    SP_ATTR_VERTEX_TEXCOORD7,
    SP_ATTR_VERTEX_COLOR = SP_ATTR_VERTEX_DIFFUSE
};

const std::map<ShaderAvailableAttributes, uint8_t> SHADER_ATTRIBUTE_SIZES = {
    { SP_ATTR_VERTEX_POSITION, 3 },
    { SP_ATTR_VERTEX_DIFFUSE, 4 },
    { SP_ATTR_VERTEX_NORMAL, 3 },
    { SP_ATTR_VERTEX_TEXCOORD0, 2},
    { SP_ATTR_VERTEX_TEXCOORD1, 2},
    { SP_ATTR_VERTEX_TEXCOORD2, 2},
    { SP_ATTR_VERTEX_TEXCOORD3, 2},
    { SP_ATTR_VERTEX_TEXCOORD4, 2},
    { SP_ATTR_VERTEX_TEXCOORD5, 2},
    { SP_ATTR_VERTEX_TEXCOORD6, 2},
    { SP_ATTR_VERTEX_TEXCOORD7, 2}
};


const std::set<ShaderAvailableAttributes> SHADER_AVAILABLE_ATTRS = {
    SP_ATTR_VERTEX_POSITION,
    SP_ATTR_VERTEX_DIFFUSE,
    SP_ATTR_VERTEX_NORMAL,
    SP_ATTR_VERTEX_TEXCOORD0,
    SP_ATTR_VERTEX_TEXCOORD1,
    SP_ATTR_VERTEX_TEXCOORD2,
    SP_ATTR_VERTEX_TEXCOORD3,
    SP_ATTR_VERTEX_TEXCOORD4,
    SP_ATTR_VERTEX_TEXCOORD5,
    SP_ATTR_VERTEX_TEXCOORD6,
    SP_ATTR_VERTEX_TEXCOORD7
};

}

namespace std {
    using kglt::ShaderAvailableAuto;

    template<>
    struct hash<ShaderAvailableAuto> {
        size_t operator()(const ShaderAvailableAuto& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };

    using kglt::ShaderAvailableAttributes;

    template<>
    struct hash<ShaderAvailableAttributes> {
        size_t operator()(const ShaderAvailableAttributes& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };
}

namespace kglt {

class GPUProgram;

typedef sig::signal<void ()> ProgramLinkedSignal;
typedef sig::signal<void (ShaderType)> ShaderCompiledSignal;

struct UniformInfo {
    unicode name;
    GLenum type;
    GLsizei size;
};

class UniformManager {
public:
    void set_int(const unicode& uniform_name, const int32_t value);
    void set_float(const unicode& uniform_name, const float value);
    void set_mat4x4(const unicode& uniform_name, const Mat4& values);
    void set_mat3x3(const unicode& uniform_name, const Mat3& values);
    void set_vec3(const unicode& uniform_name, const Vec3& values);
    void set_vec4(const unicode& uniform_name, const Vec4& values);
    void set_colour(const unicode& uniform_name, const Colour& values);
    void set_mat4x4_array(const unicode& uniform_name, const std::vector<Mat4>& matrices);

    bool uses_auto(ShaderAvailableAuto uniform) const {
        return auto_uniforms_.find(uniform) != auto_uniforms_.end();
    }

    unicode auto_variable_name(ShaderAvailableAuto auto_name) const {
        auto it = auto_uniforms_.find(auto_name);
        if(it == auto_uniforms_.end()) {
            throw std::logic_error("Specified auto is not registered");
        }

        return (*it).second;
    }

    void register_auto(ShaderAvailableAuto uniform, const unicode& var_name);
    void clear_cache() {
        uniform_cache_.clear();
    }

    UniformInfo info(const unicode& uniform_name);

private:
    friend class GPUProgram;
    GPUProgram& program_;

    UniformManager(GPUProgram& program);
    GLint locate(const unicode& uniform_name);

    std::unordered_map<unicode, UniformInfo> uniform_info_;
    std::unordered_map<unicode, GLint> uniform_cache_;

    void clear_uniform_cache();

    std::unordered_map<ShaderAvailableAuto, unicode> auto_uniforms_;
};

class AttributeManager {
public:
    int32_t locate(const unicode& attribute);
    void set_location(const unicode& attribute, int32_t location);

    void register_auto(ShaderAvailableAttributes attr, const unicode& var_name);

    unicode variable_name(ShaderAvailableAttributes attr_name) const {
        auto it = auto_attributes_.find(attr_name);
        if(it == auto_attributes_.end()) {
            throw std::logic_error("Specified attribute is not registered");
        }

        return (*it).second;
    }

    bool uses_auto(ShaderAvailableAttributes attr) const {
        return auto_attributes_.find(attr) != auto_attributes_.end();
    }

    void clear_cache() {
        attribute_cache_.clear();
    }

private:
    friend class GPUProgram;

    AttributeManager(GPUProgram& program);

    GPUProgram& program_;
    std::unordered_map<unicode, int32_t> attribute_cache_;
    std::unordered_map<ShaderAvailableAttributes, unicode> auto_attributes_;
};

class GPUProgram:
    public Managed<GPUProgram> {

public:
    GPUProgram();

    bool init() override;
    void cleanup() override;

    UniformManager& uniforms() { return uniforms_; }
    AttributeManager& attributes() { return attributes_; }

    const bool is_current() const;
    void activate();
    void set_shader_source(ShaderType type, const unicode& source);
    const bool is_complete() const;
    const bool is_compiled(ShaderType type) const;

    void compile(ShaderType type);
    void build();

    ProgramLinkedSignal& signal_linked() { return signal_linked_; }
    ShaderCompiledSignal& signal_shader_compiled() { return signal_shader_compiled_; }

    unicode md5() { return md5_shader_hash_; }
private:
    friend class UniformManager;
    friend class AttributeManager;

    void prepare_program();

    struct ShaderInfo {
        uint32_t object = 0;
        bool is_compiled = false;
        unicode source;
    };

    bool is_linked_ = false;

    uint32_t program_object_ = 0;
    std::map<ShaderType, ShaderInfo> shaders_;
    std::map<ShaderType, unicode> shader_hashes_;

    ProgramLinkedSignal signal_linked_;
    ShaderCompiledSignal signal_shader_compiled_;

    UniformManager uniforms_;
    AttributeManager attributes_;

    void link();

    //A hash of all the GLSL shaders so we can uniquely identify a program
    void rebuild_hash();
    unicode md5_shader_hash_;
};

}
#endif // GPU_PROGRAM_H
