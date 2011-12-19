#include <stdexcept>

#include "types.h"
#include "shader.h"

namespace kglt {

ShaderProgram::ShaderProgram() {

    for(uint32_t i = 0; i < MAX_SHADER_TYPES; ++i) {
        shader_ids_[i] = NullShaderID;
    }

    program_id_ = glCreateProgram();
}

ShaderProgram::~ShaderProgram() {
    try {
        for(uint32_t i = 0; i < ShaderType::MAX_SHADER_TYPES; ++i) {
            if(shader_ids_[i] != NullShaderID) glDeleteShader(shader_ids_[i]);
        }

        glDeleteProgram(program_id_);
    } catch (...) { }
}

void ShaderProgram::activate() {
    glUseProgram(program_id_);
}

void ShaderProgram::add_and_compile(ShaderType type, const std::string& source) {
    if(shader_ids_[type]) {
        glDeleteShader(shader_ids_[type]);
        shader_ids_[type] = 0;
    }

    GLenum shader_type;
    switch(type) {
        case ShaderType::VERTEX: {
            shader_type = GL_VERTEX_SHADER;
        } break;
        case ShaderType::FRAGMENT: {
            shader_type = GL_FRAGMENT_SHADER;
        } break;
        default:
            throw std::logic_error("Invalid shader type");
    }

    shader_ids_[type] = glCreateShader(shader_type);

    const char* c_str = source.c_str();
    glShaderSource(shader_ids_[type], 1, &c_str, nullptr);
    glCompileShader(shader_ids_[type]);

    glAttachShader(program_id_, shader_ids_[type]);
    glLinkProgram(program_id_);
}

GLint ShaderProgram::get_uniform_loc(const std::string& name) {
    return glGetUniformLocation(program_id_, name.c_str());
}

void ShaderProgram::set_uniform(const std::string& name, const float x) {
    glUniform1f(get_uniform_loc(name), x);
}

void ShaderProgram::set_uniform(const std::string& name, const int32_t x) {
    glUniform1i(get_uniform_loc(name), x);
}

void ShaderProgram::set_uniform(const std::string& name, const kmMat4* matrix) {
    glUniformMatrix4fv(get_uniform_loc(name), 1, false, (GLfloat*) matrix->mat);
}

void ShaderProgram::set_uniform(const std::string& name, const kmMat3* matrix) {
    glUniformMatrix3fv(get_uniform_loc(name), 1, false, (GLfloat*) matrix->mat);
}

void ShaderProgram::set_uniform(const std::string& name, const kmVec3* vec) {
    glUniform3fv(get_uniform_loc(name), 1, (GLfloat*) vec);
}

}
