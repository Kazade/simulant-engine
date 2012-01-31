#include <stdexcept>
#include <boost/format.hpp>

#include "utils/gl_error.h"
#include "kazbase/logging/logging.h"
#include "kazbase/exceptions.h"
#include "types.h"
#include "shader.h"

namespace kglt {

ShaderProgram::ShaderProgram():
    program_id_(0) {

    for(uint32_t i = 0; i < SHADER_TYPE_MAX; ++i) {
        shader_ids_[i] = NullShaderID;
    }

}

ShaderProgram::~ShaderProgram() {
    try {
        for(uint32_t i = 0; i < ShaderType::SHADER_TYPE_MAX; ++i) {
            if(shader_ids_[i] != NullShaderID) glDeleteShader(shader_ids_[i]);
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
    
    if(shader_ids_[type] != NullShaderID) {
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
    if(compiled == GL_FALSE) {
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
    
    glLinkProgram(program_id_);
    check_and_log_error(__FILE__, __LINE__);

    GLint linked = 0;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &linked);
    if(linked == GL_FALSE) {
        GLint length;
        glGetProgramiv(program_id_, GL_INFO_LOG_LENGTH, &length);
        
        std::vector<char> log;
        log.resize(length);
        
        glGetProgramInfoLog(program_id_, length, NULL, &log[0]);
        L_ERROR(std::string(log.begin(), log.end()));
    }
    assert(linked); 
}

GLint ShaderProgram::get_uniform_loc(const std::string& name) {
    GLint location = glGetUniformLocation(program_id_, name.c_str());
    if(location < 0) {
        L_INFO("Couldn't find uniform with name: " + name);
    }
    return location;
}

void ShaderProgram::set_uniform(const std::string& name, const float x) {
    glUniform1f(get_uniform_loc(name), x);
    check_and_log_error(__FILE__, __LINE__);
}

void ShaderProgram::set_uniform(const std::string& name, const int32_t x) {
    glUniform1i(get_uniform_loc(name), x);
    check_and_log_error(__FILE__, __LINE__);    
}

void ShaderProgram::set_uniform(const std::string& name, const kmMat4* matrix) {
    GLint loc = get_uniform_loc(name);
    if(loc >= 0) {
        glUniformMatrix4fv(loc, 1, false, (GLfloat*) matrix->mat);
        check_and_log_error(__FILE__, __LINE__);
    }
}

void ShaderProgram::set_uniform(const std::string& name, const kmMat3* matrix) {
    glUniformMatrix3fv(get_uniform_loc(name), 1, false, (GLfloat*) matrix->mat);
    check_and_log_error(__FILE__, __LINE__);    
}

void ShaderProgram::set_uniform(const std::string& name, const kmVec3* vec) {
    glUniform3fv(get_uniform_loc(name), 1, (GLfloat*) vec);
    check_and_log_error(__FILE__, __LINE__);    
}

}
