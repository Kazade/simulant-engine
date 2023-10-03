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

#ifndef GPU_PROGRAM_H
#define GPU_PROGRAM_H

#include <set>
#include <map>
#include <unordered_map>

#include "../../signals/signal.h"
#include "../../types.h"
#include "../../generic/property.h"
#include "../../generic/managed.h"
#include "../../utils/gl_thread_check.h"
#include "../../generic/identifiable.h"
#include "../../vertex_data.h"

#include "../glad/glad/glad.h"

#define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + (bytes))

namespace std {
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

class Renderer;
class GPUProgram;

typedef sig::signal<void ()> ProgramLinkedSignal;
typedef sig::signal<void (ShaderType)> ShaderCompiledSignal;

struct UniformInfo {
    unicode name;
    GLenum type;
    GLsizei size;
};


class GPUProgram:
    public RefCounted<GPUProgram>,
    public generic::Identifiable<GPUProgramID> {

public:
    friend class Renderer;

    GPUProgram(const GPUProgramID& id, Renderer* renderer, const std::string& vertex_source, const std::string& fragment_source);
    GPUProgram(const GPUProgram&) = delete;
    GPUProgram& operator=(const GPUProgram&) = delete;

    bool on_init() override;
    void on_clean_up() override;

    bool is_current() const;
    void activate();

    bool is_complete() const;
    bool is_compiled(ShaderType type) const;

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

    GLint locate_uniform(const std::string& name, bool fail_silently=false);
    GLint locate_attribute(const std::string& name, bool fail_silently=false);
    void set_uniform_location(const std::string& name, GLint location);
    void set_attribute_location(const std::string& name, GLint location);

    UniformInfo uniform_info(const std::string& uniform_name);

    void clear_cache() {
        uniform_cache_.clear();
    }

    void set_uniform_int(const int32_t loc, const int32_t value);
    void set_uniform_mat4x4(const int32_t loc, const Mat4& values);
    void set_uniform_color(const int32_t loc, const Color& values);
    void set_uniform_vec4(const int32_t loc, const Vec4& values);
    void set_uniform_float(const int32_t loc, const float value);

    void set_uniform_int(const std::string& uniform_name, const int32_t value, bool fail_silently=false);
    void set_uniform_float(const std::string& uniform_name, const float value, bool fail_silently=false);
    void set_uniform_mat4x4(const std::string& uniform_name, const Mat4& values);
    void set_uniform_mat3x3(const std::string& uniform_name, const Mat3& values);
    void set_uniform_vec3(const std::string& uniform_name, const Vec3& values);
    void set_uniform_vec4(const std::string& uniform_name, const Vec4& values);
    void set_uniform_color(const std::string& uniform_name, const Color& values);
    void set_uniform_mat4x4_array(const std::string& uniform_name, const std::vector<Mat4>& matrices);

    void relink() {
        if(needs_relink_) {
            link();
            needs_relink_ = false;
        }
    }

    GLuint program_object() const { return program_object_; }
    void prepare_program();

    void _set_renderer_specific_id(uint32_t id) {
        renderer_id_ = id;
    }

    uint32_t _renderer_specific_id() const {
        return renderer_id_;
    }

private:
    friend class ::ShaderTest;

    Renderer* renderer_;

    std::unordered_map<unicode, UniformInfo> uniform_info_;

    void rebuild_uniform_info();
    void set_shader_source(ShaderType type, const std::string &source);

    bool is_linked_ = false;
    bool needs_relink_ = true;

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

    void link(bool force=false);

    uint32_t renderer_id_ = 0;
};


}
#endif // GPU_PROGRAM_H
