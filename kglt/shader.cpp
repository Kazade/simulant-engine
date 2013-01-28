#include <GLee.h>
#include <stdexcept>
#include <boost/format.hpp>

#include "utils/gl_error.h"
#include "kglt/kazbase/logging/logging.h"
#include "kglt/kazbase/exceptions.h"
#include "kglt/kazbase/list_utils.h"
#include "types.h"
#include "shader.h"

namespace kglt {

ShaderParams::ShaderParams(ShaderProgram& parent):
    program_(parent) {

}

void ShaderParams::register_auto(ShaderAvailableAuto auto_const, const std::string& uniform_name) {
    auto_uniforms_[auto_const] = uniform_name;
}

void ShaderParams::register_attribute(ShaderAvailableAttributes attr_const, const std::string& attrib_name) {
    auto_attributes_[attr_const] = attrib_name;
}

void ShaderParams::set_int(const std::string& uniform_name, const int32_t value) {
    program_.set_uniform(uniform_name, (int32_t) value);
}

void ShaderParams::set_float(const std::string& uniform_name, const float value) {
    program_.set_uniform(uniform_name, (float) value);
}

void ShaderParams::set_mat4x4(const std::string& uniform_name, const kmMat4& values) {
    program_.set_uniform(uniform_name, &values);
}

void ShaderParams::set_mat3x3(const std::string& uniform_name, const kmMat3& values) {
    program_.set_uniform(uniform_name, &values);
}

void ShaderParams::set_vec3(const std::string& uniform_name, const kmVec3& values) {
    program_.set_uniform(uniform_name, &values);
}

void ShaderParams::set_vec4(const std::string& uniform_name, const kmVec4& values) {
    program_.set_uniform(uniform_name, &values);
}

void ShaderParams::set_colour(const std::string& uniform_name, const Colour& values) {
    kmVec4 tmp;
    kmVec4Fill(&tmp, values.r, values.g, values.b, values.a);
    set_vec4(uniform_name, tmp);
}

void ShaderParams::set_mat4x4_array(const std::string& uniform_name, const std::vector<kmMat4>& matrices) {
    program_.set_uniform(uniform_name, matrices);
}

ShaderProgram::ShaderProgram(ResourceManager* resource_manager, ShaderID id):    
    Resource(resource_manager),
    generic::Identifiable<ShaderID>(id),
    program_id_(0),
    params_(*this) {

    for(uint32_t i = 0; i < SHADER_TYPE_MAX; ++i) {
        shader_ids_[i] = 0;
    }
}

ShaderProgram::~ShaderProgram() {
    try {
        for(uint32_t i = 0; i < ShaderType::SHADER_TYPE_MAX; ++i) {
            if(shader_ids_[i] != 0) {
                glDeleteShader(shader_ids_[i]);
            }
        }

        if(program_id_) {
            glDeleteProgram(program_id_);
        }
        check_and_log_error(__FILE__, __LINE__);
    } catch (...) { }
}

void ShaderProgram::activate() {

    glUseProgram(program_id_);
    check_and_log_error(__FILE__, __LINE__);
}

void ShaderProgram::bind_attrib(uint32_t idx, const std::string& name) {
    glBindAttribLocation(program_id_, idx, name.c_str());
    check_and_log_error(__FILE__, __LINE__);
}

void ShaderProgram::add_and_compile(ShaderType type, const std::string& source) {
    check_and_log_error(__FILE__, __LINE__);

    if(program_id_ == 0) {
        program_id_ = glCreateProgram();
        check_and_log_error(__FILE__, __LINE__);
    }

    if(shader_ids_[type] != 0) {
        glDeleteShader(shader_ids_[type]);
        shader_ids_[type] = 0;
        check_and_log_error(__FILE__, __LINE__);
    }

    GLuint shader_type;
    switch(type) {
        case ShaderType::SHADER_TYPE_VERTEX: {
            L_DEBUG("Adding vertex shader");
            shader_type = GL_VERTEX_SHADER;
        } break;
        case ShaderType::SHADER_TYPE_FRAGMENT: {
            L_DEBUG("Adding fragment shader");
            shader_type = GL_FRAGMENT_SHADER;
        } break;
        default:
            throw std::logic_error("Invalid shader type");
    }
    check_and_log_error(__FILE__, __LINE__);
    GLuint shader = glCreateShader(shader_type);
    check_and_log_error(__FILE__, __LINE__);
    shader_ids_[type] = shader;

    const char* c_str = source.c_str();
    glShaderSource(shader, 1, &c_str, nullptr);
    check_and_log_error(__FILE__, __LINE__);

    glCompileShader(shader);
    check_and_log_error(__FILE__, __LINE__);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> log;
        log.resize(length);

        glGetShaderInfoLog(shader, length, NULL, &log[0]);
        L_ERROR(std::string(log.begin(), log.end()));
    }
    assert(compiled);
    assert(program_id_);
    assert(shader);

    check_and_log_error(__FILE__, __LINE__);
    glAttachShader(program_id_, shader);
    check_and_log_error(__FILE__, __LINE__);

    relink();
}

void ShaderProgram::relink() {
    glLinkProgram(program_id_);
    check_and_log_error(__FILE__, __LINE__);

    GLint linked = 0;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &linked);
    if(!linked) {
        GLint length;
        glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &length);

        std::vector<char> log;
        log.resize(length);

        glGetProgramInfoLog(program_id_, length, NULL, &log[0]);
        L_ERROR(std::string(log.begin(), log.end()));
    }
    assert(linked);
}

int32_t ShaderProgram::get_attrib_loc(const std::string& name) {
    GLint location = glGetAttribLocation(program_id_, name.c_str());
    if(location < 0) {
        L_WARN("No attribute with name: " + name);
    }

    return location;
}

int32_t ShaderProgram::get_uniform_loc(const std::string& name) {
    if(container::contains(cached_uniform_locations_, name)) {
        return cached_uniform_locations_[name];
    }

    GLint location = glGetUniformLocation(program_id_, name.c_str());
    if(location < 0) {
        L_WARN("No uniform with name: " + name);
    }

    cached_uniform_locations_[name] = location;
    return location;
}

bool ShaderProgram::has_uniform(const std::string& name) {
    return get_uniform_loc(name) != -1;
}

void ShaderProgram::set_uniform(const std::string& name, const float x) {
    activate();
    glUniform1f(get_uniform_loc(name), x);
    check_and_log_error(__FILE__, __LINE__);
}

void ShaderProgram::set_uniform(const std::string& name, const int32_t x) {
    activate();
    glUniform1i(get_uniform_loc(name), x);
    check_and_log_error(__FILE__, __LINE__);
}

void ShaderProgram::set_uniform(const std::string& name, const kmMat4* matrix) {
    activate();
    GLint loc = get_uniform_loc(name);
    if(loc >= 0) {
        float mat[16];
        unsigned char i = 16;
        while(i--) { mat[i] = (float) matrix->mat[i]; }
        glUniformMatrix4fv(loc, 1, false, (GLfloat*)mat);
        check_and_log_error(__FILE__, __LINE__);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const kmMat3* matrix) {
    activate();
    GLint loc = get_uniform_loc(name);
    if(loc >= 0) {
        float mat[9];
        unsigned char i = 9;
        while(i--) { mat[i] = (float) matrix->mat[i]; }
        glUniformMatrix3fv(get_uniform_loc(name), 1, false, (GLfloat*)mat);
        check_and_log_error(__FILE__, __LINE__);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const kmVec3* vec) {
    activate();
    GLint loc = get_uniform_loc(name);
    if(loc >= 0) {
        glUniform3fv(get_uniform_loc(name), 1, (GLfloat*) vec);
        check_and_log_error(__FILE__, __LINE__);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const kmVec4* vec) {
    activate();
    GLint loc = get_uniform_loc(name);
    if(loc >= 0) {
        glUniform4fv(get_uniform_loc(name), 1, (GLfloat*) vec);
        check_and_log_error(__FILE__, __LINE__);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const std::vector<kmMat4>& matrices) {
    activate();
    GLint loc = get_uniform_loc(name);
    if(loc >= 0) {
        glUniformMatrix4fv(loc, matrices.size(), false, (GLfloat*) &matrices[0]);
        check_and_log_error(__FILE__, __LINE__);
    }
}

}
