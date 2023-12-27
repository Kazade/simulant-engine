//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//


#include "../../utils/gl_error.h"
#include "../../utils/hash/md5.h"
#include "gpu_program.h"
#include "../renderer.h"
#include "../../generic/raii.h"

namespace smlt {


UniformInfo GPUProgram::uniform_info(const std::string& uniform_name) {
    /*
     *  Returns information about the named uniform
     */
    auto it = uniform_info_.find(uniform_name);

    if(it == uniform_info_.end()) {
        throw std::logic_error("Couldn't find info for: " + uniform_name);
    }

    return (*it).second;
}

GLint GPUProgram::locate_uniform(const std::string& uniform_name, bool fail_silently) {
    /* Don't do anything costly if we already know where the uniform is */
    auto it = uniform_cache_.find(uniform_name);
    if(it != uniform_cache_.end()) {
        return (*it).second;
    }

    /* Make sure we always rebind the currently bound gpu program when
     * we leave this function */
    auto current = renderer_->current_gpu_program_id();
    raii::Finally then([&]() {
        if(current && current != id()) {
            auto program = renderer_->gpu_program(current);
            if(program) {
                program->activate();
            } else {
                S_WARN(
                    "Program {0} vanished while locating uniforms for program {1}",
                    current, id()
                );
            }
        }
    });

    if(current != id()) {
        activate();
    }

    if(!is_complete()) {
        S_ERROR("Attempted to modify a uniform without making the program complete");
        throw std::logic_error("Attempted to access uniform on a GPU program that is not complete");
    }

    std::string name = uniform_name;

    //L_DEBUG(_u("Looking up uniform with name {0} in program {1}").format(name, program_.program_object_));
    GLint location = _GLCheck<GLint>(__func__, glGetUniformLocation, program_object_, name.c_str());

    if(location < 0) {
        if(!fail_silently) {
            S_ERROR("Couldn't find uniform {0}. Has it been optimized into non-existence?", name);
        }
    }

    uniform_cache_[uniform_name] = location;
    return location;
}

void GPUProgram::set_uniform_int(const int32_t loc, const int32_t value) {
    assert(loc >= 0);
    GLCheck(glUniform1i, loc, value);
}

void GPUProgram::set_uniform_int(const std::string& uniform_name, const int32_t value, bool fail_silently) {
    GLint loc = locate_uniform(uniform_name, fail_silently);
    if(loc > -1) {
        GLCheck(glUniform1i, loc, value);
    }
}

void GPUProgram::set_uniform_float(const int32_t loc, const float value) {
    assert(loc >= 0);
    GLCheck(glUniform1f, loc, value);
}

void GPUProgram::set_uniform_float(const std::string& uniform_name, const float value, bool fail_silently) {
    int32_t loc = locate_uniform(uniform_name, fail_silently);
    if(loc > -1) {
        GLCheck(glUniform1f, loc, value);
    }
}

void GPUProgram::set_uniform_mat4x4(const int32_t loc, const Mat4& matrix) {
    assert(loc >= 0);
    GLCheck(glUniformMatrix4fv, loc, 1, false, (GLfloat*)matrix.data());
}

void GPUProgram::set_uniform_mat4x4(const std::string& uniform_name, const Mat4& matrix) {
    int32_t loc = locate_uniform(uniform_name);
    if(loc > -1) {
        GLCheck(glUniformMatrix4fv, loc, 1, false, (GLfloat*)matrix.data());
    }
}

void GPUProgram::set_uniform_mat3x3(const std::string& uniform_name, const Mat3& matrix) {
    int32_t loc = locate_uniform(uniform_name);
    if(loc > -1) {
        GLCheck(glUniformMatrix3fv, loc, 1, false, (GLfloat*)matrix.data());
    }
}

void GPUProgram::set_uniform_vec3(const std::string& uniform_name, const Vec3& values) {
    int32_t loc = locate_uniform(uniform_name);
    if(loc > -1) {
        GLCheck(glUniform3fv, loc, 1, (GLfloat*) &values);
    }
}

void GPUProgram::set_uniform_vec4(const int32_t loc, const Vec4& values) {
    assert(loc >= 0);
    GLCheck(glUniform4fv, loc, 1, (GLfloat*) &values);
}

void GPUProgram::set_uniform_vec4(const std::string& uniform_name, const Vec4& values) {
    int32_t loc = locate_uniform(uniform_name);
    if(loc > -1) {
        GLCheck(glUniform4fv, loc, 1, (GLfloat*) &values);
    }
}

void GPUProgram::set_uniform_colour(const int32_t loc, const Colour& values) {
    set_uniform_vec4(loc, Vec4(values.r, values.g, values.b, values.a));
}

void GPUProgram::set_uniform_colour(const std::string& uniform_name, const Colour& values) {
    Vec4 tmp(values.r, values.g, values.b, values.a);
    set_uniform_vec4(uniform_name, tmp);
}

void GPUProgram::set_uniform_mat4x4_array(const std::string& uniform_name, const std::vector<Mat4>& matrices) {
    int32_t loc = locate_uniform(uniform_name);
    GLCheck(glUniformMatrix4fv, loc, matrices.size(), false, (GLfloat*) &matrices[0]);
}

void GPUProgram::rebuild_uniform_info() {
    //FIXME: Make this only happen when debugging
    //DEBUG info!
    GLint count;
    glGetProgramiv(program_object_, GL_ACTIVE_UNIFORMS, &count);

    uniform_info_.clear();

    char buf[256];
    GLsizei buf_count;
    GLint size;
    GLenum type;

    for(int i = 0; i < count; ++i) {
        glGetActiveUniform(program_object_, (GLuint) i,  256,  &buf_count,  &size,  &type,  buf);

        std::string name(buf, buf + buf_count);

        UniformInfo info;
        info.name = name;
        info.size = size;
        info.type = type;

        uniform_info_[info.name] = info;
    }
}


GPUProgram::GPUProgram(const GPUProgramID &id, Renderer* renderer, const std::string &vertex_source, const std::string &fragment_source):
    generic::Identifiable<GPUProgramID>(id),
    renderer_(renderer),
    program_object_(0) {

    set_shader_source(SHADER_TYPE_VERTEX, vertex_source);
    set_shader_source(SHADER_TYPE_FRAGMENT, fragment_source);
}

GLint GPUProgram::locate_attribute(const std::string &attribute, bool fail_silently) {
    auto it = attribute_cache_.find(attribute);
    if(it != attribute_cache_.end()) {
        return (*it).second;
    }

    if(!is_complete()) {
        throw std::logic_error("Attempted to access attribute on a GPU program that is not complete");
    }

    GLint location = _GLCheck<GLint>(__func__, glGetAttribLocation, program_object_, attribute.c_str());
    if(location < 0) {
        if(!fail_silently) {
            S_ERROR("Unable to find attribute with name {0}", attribute);
        }
    }

    attribute_cache_[attribute] = location;

    return location;
}

void GPUProgram::set_attribute_location(const std::string& attribute, GLint location) {
    auto it = attribute_cache_.find(attribute);
    if(it != attribute_cache_.end() && (*it).second == location) {
        return;
    }

    //No completeness check, glBindAttribLocation can be called at any time
    GLCheck(glBindAttribLocation, program_object_, location, attribute.c_str());

    //Is this always true? Can we just assume that the location was given to that attribute?
    //The docs don't seem to suggest that it can fail...
    attribute_cache_[attribute] = location;

    // Once we change the attributes they won't take effect until we relink
    needs_relink_ = true;
}

bool GPUProgram::is_current() const {
    GLint current = 0;
    GLCheck(glGetIntegerv, GL_CURRENT_PROGRAM, &current);
    return program_object_ && GLuint(current) == program_object_;
}

void GPUProgram::activate() {
    assert(program_object_);

    GLCheck(glUseProgram, program_object_);
}

void GPUProgram::prepare_program() {
    if(program_object_) {
        return;
    }

    program_object_ = _GLCheck<GLuint>(__func__, glCreateProgram);

    S_DEBUG("Created program {0}", program_object_);
}

bool GPUProgram::init() {
    return true;
}

void GPUProgram::clean_up()  {
    if(GLThreadCheck::is_current() && program_object_) {
        S_DEBUG("Destroying GPU program: {0}", program_object_);

        for(auto obj: this->shaders_) {
            if(obj.second.object) {
                GLCheck(glDeleteShader, obj.second.object);
                obj.second.object = 0;
            }
        }

        if(is_current()) {
            //If we are currently using this program, then switch back to no program!
            GLCheck(glUseProgram, 0);
        }

        GLCheck(glDeleteProgram, program_object_);

        program_object_ = 0;
    }
}

GLenum shader_type_to_glenum(ShaderType type) {
    switch(type) {
        case SHADER_TYPE_VERTEX: return GL_VERTEX_SHADER;
        case SHADER_TYPE_FRAGMENT: return GL_FRAGMENT_SHADER;
    default:
        throw std::logic_error("Unsupported shader type");
    }
}

void GPUProgram::set_shader_source(ShaderType type, const std::string& source) {
    if(source.empty()) {
        throw std::logic_error("Tried to set shader source to an empty string");
    }

    /* FIXME: This is a bit hacky! */
    const char* shader_version = (renderer_->name() == "gles2x") ? "100" : "120";

    ShaderInfo new_shader;

    // FIXME: Named formatting would be much nicer...somehow
    new_shader.source = _F(source).format(shader_version);

    is_linked_ = false; //We're no longer linked
    shaders_[type] = new_shader;
    shader_hashes_[type] = hashlib::MD5(source).hex_digest();
    rebuild_hash();
}


void GPUProgram::compile(ShaderType type) {
    auto& info = shaders_.at(type);
    if(info.is_compiled) {
        return;
    }

    if(!info.object) {
        info.object = _GLCheck<GLuint>(__func__, glCreateShader, shader_type_to_glenum(type));
    }

    assert(info.object); //Make sure we have a shader object

    std::string encoded_string = info.source;
    const char* c_str = encoded_string.c_str();
    GLCheck(glShaderSource, info.object, 1, &c_str, nullptr);
    GLCheck(glCompileShader, info.object);

    GLint compiled = 0;
    GLCheck(glGetShaderiv, info.object, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        //Shit went down
        GLint length;
        GLCheck(glGetShaderiv, info.object, GL_INFO_LOG_LENGTH, &length);

        if(length < 0) {
            S_ERROR("Unable to get the info log for the errornous shader, are you calling from the right thread?");
            throw std::runtime_error("CRITICAL: Unable to get GLSL log");
        }

        std::vector<char> log;
        log.resize(length);

        GLCheck(glGetShaderInfoLog, info.object, length, (GLsizei*)NULL, &log[0]);
        std::string error_log(log.begin(), log.end());

        S_ERROR(error_log);
        throw std::runtime_error("Unable to compile shader, check the error log for details");
    }

    prepare_program(); //Make sure we have a program object before attaching the shader

    GLCheck(glAttachShader, program_object_, info.object);
    info.is_compiled = true;
}

void GPUProgram::build() {
    //Important! Do nothing if we're already built
    if(is_complete()) {
        return;
    }

    if(!GLThreadCheck::is_current()) {
        throw std::logic_error("Attempting to build a GPU program in the wrong thread");
    }

    prepare_program();

    for(auto p: shaders_) {
        compile(p.first); //Compile each shader if necessary
    }

    link(); //Now link the program
}

bool GPUProgram::is_complete() const {
    //Vertex and Fragment shader are required
    if(!program_object_ || !shaders_.count(SHADER_TYPE_VERTEX) || !shaders_.count(SHADER_TYPE_FRAGMENT)) {
        return false;
    }

    //If we aren't linked, we aren't complete
    if(is_linked_) {
        //Just make doubly sure that we're consistent with ourselves
        assert(shaders_.at(SHADER_TYPE_VERTEX).is_compiled);
        assert(shaders_.at(SHADER_TYPE_FRAGMENT).is_compiled);
        return true;
    }

    return false;
}

bool GPUProgram::is_compiled(ShaderType type) const {
    return shaders_.at(type).is_compiled;
}

void GPUProgram::rebuild_hash() {
    hashlib::MD5 combined_hash;

    for(auto p: shader_hashes_) {
        combined_hash.update(p.second);
    }

    md5_shader_hash_ = combined_hash.hex_digest();
}


void GPUProgram::link(bool force) {
    if(!force && !needs_relink_) {
        return;
    }

    prepare_program();

    assert(shaders_.at(SHADER_TYPE_VERTEX).is_compiled);
    assert(shaders_.at(SHADER_TYPE_FRAGMENT).is_compiled);

    //Link the program
    GLCheck(glLinkProgram, program_object_);

    //Check everything was hunky-dory
    GLint linked = 0;
    GLCheck(glGetProgramiv, program_object_, GL_LINK_STATUS, &linked);

    if(!linked) {
        GLint length;
        GLCheck(glGetProgramiv, program_object_, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> log;
        log.resize(length);

        GLCheck(glGetProgramInfoLog, program_object_, length, (GLsizei*)NULL, &log[0]);

        std::string error_log(log.begin(), log.end());
        S_ERROR(error_log);

        throw std::runtime_error("Couldn't link the GPU program. See error log for details.");
    }

    S_DEBUG("Linked program {0}", program_object_);

    // Rebuild the uniform information for debugging
    rebuild_uniform_info();
    uniform_cache_.clear();

    is_linked_ = true;
    needs_relink_ = false;
    signal_linked_();
}

}
