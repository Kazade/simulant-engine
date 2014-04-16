#ifndef GPU_PROGRAM_H
#define GPU_PROGRAM_H

#include <unordered_map>
#include <kazbase/signals3/signals3.hpp>

#include "types.h"
#include "generic/managed.h"
#include "utils/gl_thread_check.h"

namespace kglt {

class GPUProgram;

typedef sig::signal<void ()> ProgramLinkedSignal;
typedef sig::signal<void (ShaderType)> ShaderCompiledSignal;

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

private:
    friend class GPUProgram;
    GPUProgram& program_;

    UniformManager(GPUProgram& program);
    int32_t locate_uniform(const unicode& uniform_name);

    std::unordered_map<unicode, int32_t> uniform_cache_;

    void clear_uniform_cache();
};

class AttributeManager {
public:
    int32_t get_location(const unicode& attribute);
    void set_location(const unicode& attribute, int32_t location);

private:
    friend class GPUProgram;

    AttributeManager(GPUProgram& program);

    GPUProgram& program_;
    std::unordered_map<unicode, int32_t> attribute_cache_;
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

private:
    friend class UniformManager;
    friend class AttributeManager;

    struct ShaderInfo {
        uint32_t object = 0;
        bool is_compiled = false;
        unicode source;
    };

    bool is_linked_ = false;

    uint32_t program_object_ = 0;
    std::map<ShaderType, ShaderInfo> shaders_;

    ProgramLinkedSignal signal_linked_;
    ShaderCompiledSignal signal_shader_compiled_;

    UniformManager uniforms_;
    AttributeManager attributes_;

    void link();
};

}
#endif // GPU_PROGRAM_H
