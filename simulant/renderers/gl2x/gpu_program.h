#ifndef GPU_PROGRAM_H
#define GPU_PROGRAM_H

#include <set>
#include <map>
#include <unordered_map>

#include "../../deps/kazsignal/kazsignal.h"
#include "../../types.h"
#include "../../generic/property.h"
#include "../../generic/managed.h"
#include "../../utils/gl_thread_check.h"
#include "../../generic/identifiable.h"
#include "../../vertex_data.h"

#include "glad/glad/glad.h"

#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + (bytes))

namespace smlt {

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
    SP_AUTO_MATERIAL_TEX_MATRIX4,
    SP_AUTO_MATERIAL_TEX_MATRIX5,
    SP_AUTO_MATERIAL_TEX_MATRIX6,
    SP_AUTO_MATERIAL_TEX_MATRIX7,
    SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS,
    SP_AUTO_MATERIAL_POINT_SIZE,
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


enum ShaderAvailableAttributes {
    SP_ATTR_VERTEX_POSITION,    
    SP_ATTR_VERTEX_NORMAL,
    SP_ATTR_VERTEX_TEXCOORD0,
    SP_ATTR_VERTEX_TEXCOORD1,
    SP_ATTR_VERTEX_TEXCOORD2,
    SP_ATTR_VERTEX_TEXCOORD3,
    SP_ATTR_VERTEX_DIFFUSE,
    SP_ATTR_VERTEX_SPECULAR
};


VertexAttributeType convert(ShaderAvailableAttributes attr);

}

namespace std {
    using smlt::ShaderAvailableAuto;

    template<>
    struct hash<ShaderAvailableAuto> {
        size_t operator()(const ShaderAvailableAuto& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };

    using smlt::ShaderAvailableAttributes;

    template<>
    struct hash<ShaderAvailableAttributes> {
        size_t operator()(const ShaderAvailableAttributes& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };

    using smlt::ShaderType;

    template<>
    struct hash<ShaderType> {
        size_t operator()(const ShaderType& a) const {
            hash<int32_t> make_hash;
            return make_hash(int32_t(a));
        }
    };
}

class ShaderTest;

namespace smlt {

const std::set<ShaderAvailableAuto> SHADER_AVAILABLE_AUTOS = {
    SP_AUTO_MODELVIEW_PROJECTION_MATRIX,
    SP_AUTO_MODELVIEW_MATRIX,
    SP_AUTO_PROJECTION_MATRIX,
    SP_AUTO_MATERIAL_DIFFUSE,
    SP_AUTO_MATERIAL_SPECULAR,
    SP_AUTO_MATERIAL_AMBIENT,
    SP_AUTO_MATERIAL_SHININESS,
    SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS,
    SP_AUTO_MATERIAL_POINT_SIZE,
};

const std::set<ShaderAvailableAttributes> SHADER_AVAILABLE_ATTRS = {
    SP_ATTR_VERTEX_POSITION,
    SP_ATTR_VERTEX_DIFFUSE,
    SP_ATTR_VERTEX_NORMAL,
    SP_ATTR_VERTEX_TEXCOORD0,
    SP_ATTR_VERTEX_TEXCOORD1,
    SP_ATTR_VERTEX_TEXCOORD2,
    SP_ATTR_VERTEX_TEXCOORD3,
};

}

namespace smlt {

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
    ~UniformManager() {

    }

    bool uses_auto(ShaderAvailableAuto uniform) const {
        return auto_uniforms_.find(uniform) != auto_uniforms_.end();
    }

    std::string auto_variable_name(ShaderAvailableAuto auto_name) const {
        auto it = auto_uniforms_.find(auto_name);
        if(it == auto_uniforms_.end()) {
            throw std::logic_error("Specified auto is not registered");
        }

        return (*it).second;
    }

    void register_auto(ShaderAvailableAuto uniform, const std::string& var_name);
    const std::unordered_map<ShaderAvailableAuto, std::string>& auto_uniforms() const {
        return auto_uniforms_;
    }

private:
    friend class GPUProgramInstance;
    GPUProgram* program_;

    UniformManager(GPUProgram* program);
    std::unordered_map<ShaderAvailableAuto, std::string> auto_uniforms_;
};

class AttributeManager {
public:
    void register_auto(ShaderAvailableAttributes attr, const std::string &var_name);

    std::string variable_name(ShaderAvailableAttributes attr_name) const {
        auto it = auto_attributes_.find(attr_name);
        if(it == auto_attributes_.end()) {
            throw std::logic_error("Specified attribute is not registered");
        }

        return (*it).second;
    }

    bool uses_auto(ShaderAvailableAttributes attr) const {
        return auto_attributes_.find(attr) != auto_attributes_.end();
    }

    const std::unordered_map<ShaderAvailableAttributes, std::string>& auto_attributes() const {
        return auto_attributes_;
    }

private:
    friend class GPUProgramInstance;

    AttributeManager(GPUProgram* program);

    GPUProgram* program_;
    std::unordered_map<ShaderAvailableAttributes, std::string> auto_attributes_;
};

class GPUProgram:
    public Managed<GPUProgram>,
    public generic::Identifiable<ShaderID> {

public:
    GPUProgram(const std::string& vertex_source, const std::string& fragment_source);
    GPUProgram(const GPUProgram&) = delete;
    GPUProgram& operator=(const GPUProgram&) = delete;

    bool init() override;
    void cleanup() override;

    const bool is_current() const;
    void activate();

    const bool is_complete() const;
    const bool is_compiled(ShaderType type) const;

    void compile(ShaderType type);
    void build();

    ProgramLinkedSignal& signal_linked() { return signal_linked_; }
    ShaderCompiledSignal& signal_shader_compiled() { return signal_shader_compiled_; }

    std::string md5() { return md5_shader_hash_; }

    struct ShaderInfo {
        uint32_t object = 0;
        bool is_compiled = false;
        std::string source;
    };

    const std::unordered_map<ShaderType, ShaderInfo> shader_infos() const { return shaders_; }

    GLint locate_uniform(const std::string& name);
    GLint locate_attribute(const std::string& name);
    void set_uniform_location(const std::string& name, GLint location);
    void set_attribute_location(const std::string& name, GLint location);

    UniformInfo uniform_info(const std::string& uniform_name);

    void clear_cache() {
        uniform_cache_.clear();
    }

    void set_uniform_int(const std::string& uniform_name, const int32_t value);
    void set_uniform_float(const std::string& uniform_name, const float value);
    void set_uniform_mat4x4(const std::string& uniform_name, const Mat4& values);
    void set_uniform_mat3x3(const std::string& uniform_name, const Mat3& values);
    void set_uniform_vec3(const std::string& uniform_name, const Vec3& values);
    void set_uniform_vec4(const std::string& uniform_name, const Vec4& values);
    void set_uniform_colour(const std::string& uniform_name, const Colour& values);
    void set_uniform_mat4x4_array(const std::string& uniform_name, const std::vector<Mat4>& matrices);

    void relink() {
        if(needs_relink_) {
            link();
            needs_relink_ = false;
        }
    }

    GLuint program_object() const { return program_object_; }
    void prepare_program();

private:
    friend class ::ShaderTest;

    std::unordered_map<unicode, UniformInfo> uniform_info_;

    void rebuild_uniform_info();
    void set_shader_source(ShaderType type, const std::string &source);

    bool is_linked_ = false;
    bool needs_relink_ = false;

    uint32_t program_object_ = 0;
    std::unordered_map<ShaderType, ShaderInfo> shaders_;
    std::unordered_map<ShaderType, std::string> shader_hashes_;

    ProgramLinkedSignal signal_linked_;
    ShaderCompiledSignal signal_shader_compiled_;

    //A hash of all the GLSL shaders so we can uniquely identify a program
    void rebuild_hash();
    std::string md5_shader_hash_;

    std::unordered_map<std::string, GLint> uniform_cache_;
    std::unordered_map<std::string, int32_t> attribute_cache_;

    void link();

    static uint32_t shader_id_counter_;
};

class GPUProgramInstance : public Managed<GPUProgramInstance> {
public:
    GPUProgramInstance(GPUProgram::ptr program):
        program_(program),
        uniforms_(program.get()),
        attributes_(program.get()){

        assert(program);
    }

    Property<GPUProgramInstance, GPUProgram> program = { this, &GPUProgramInstance::program_ };
    Property<GPUProgramInstance, UniformManager> uniforms = { this, &GPUProgramInstance::uniforms_ };
    Property<GPUProgramInstance, AttributeManager> attributes = { this, &GPUProgramInstance::attributes_ };

    // Internal, used for cloning program instances
    GPUProgram::ptr _program_as_shared_ptr() { return program_; }

    void set_gpu_program(GPUProgram::ptr new_program) {
        program_ = new_program;
        uniforms_.program_ = program_.get();
        attributes_.program_ = program_.get();
    }

private:
    friend class UniformManager;
    friend class AttributeManager;

    GPUProgram::ptr program_;
    UniformManager uniforms_;
    AttributeManager attributes_;
};


}
#endif // GPU_PROGRAM_H
