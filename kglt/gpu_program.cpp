#include <kazbase/exceptions.h>
#include <kazbase/hash/md5.h>

#include "utils/gl_error.h"
#include "gpu_program.h"

namespace kglt {

UniformManager::UniformManager(GPUProgram &program):
    program_(program) {

    //Always clear the uniform cache when the program is relinked
    program.signal_linked().connect(std::bind(&UniformManager::clear_uniform_cache, this));
}

UniformInfo UniformManager::info(const unicode& uniform_name) {
    /*
     *  Returns information about the named uniform
     */
    auto it = uniform_info_.find(uniform_name);

    if(it == uniform_info_.end()) {
        throw DoesNotExist<UniformInfo>(_u("Couldn't find info for {0}").format(uniform_name));
    }

    return (*it).second;
}

GLint UniformManager::locate(const unicode& uniform_name) {
    if(!program_.is_current()) {
        L_ERROR("Attempted to modify a uniform without making the program active");
        throw LogicError("Attempted to modify GPU program object without making it active");
    }

    if(!program_.is_complete()) {
        L_ERROR("Attempted to modify a uniform without making the program complete");
        throw LogicError("Attempted to access uniform on a GPU program that is not complete");
    }

    auto it = uniform_cache_.find(uniform_name);
    if(it != uniform_cache_.end()) {
        return (*it).second;
    }

    std::string name = uniform_name.encode();

    L_DEBUG(_u("Looking up uniform with name {0} in program {1}").format(name, program_.program_object_));
    GLint location = _GLCheck<GLint>(__func__, glGetUniformLocation, program_.program_object_, name.c_str());

    if(location < 0) {
        throw LogicError(_u("Couldn't find uniform {0}. Has it been optimized into non-existence?").format(uniform_name).encode());
    }

    uniform_cache_[uniform_name] = location;
    return location;
}

void UniformManager::set_int(const unicode& uniform_name, const int32_t value) {
    GLint loc = locate(uniform_name);
    GLCheck(glUniform1i, loc, value);
}

void UniformManager::set_float(const unicode& uniform_name, const float value) {
    int32_t loc = locate(uniform_name);
    GLCheck(glUniform1f, loc, value);
}

void UniformManager::set_mat4x4(const unicode& uniform_name, const Mat4& matrix) {
    int32_t loc = locate(uniform_name);
    float mat[16];
    unsigned char i = 16;
    while(i--) { mat[i] = (float) matrix.mat[i]; }
    GLCheck(glUniformMatrix4fv, loc, 1, false, (GLfloat*)mat);
}

void UniformManager::set_mat3x3(const unicode& uniform_name, const Mat3& matrix) {
    int32_t loc = locate(uniform_name);
    float mat[9];
    unsigned char i = 9;
    while(i--) { mat[i] = (float) matrix.mat[i]; }
    GLCheck(glUniformMatrix3fv, loc, 1, false, (GLfloat*)mat);
}

void UniformManager::set_vec3(const unicode& uniform_name, const Vec3& values) {
    int32_t loc = locate(uniform_name);
    GLCheck(glUniform3fv, loc, 1, (GLfloat*) &values);
}

void UniformManager::set_vec4(const unicode& uniform_name, const Vec4& values) {
    int32_t loc = locate(uniform_name);
    GLCheck(glUniform4fv, loc, 1, (GLfloat*) &values);
}

void UniformManager::set_colour(const unicode& uniform_name, const Colour& values) {
    Vec4 tmp;
    kmVec4Fill(&tmp, values.r, values.g, values.b, values.a);
    set_vec4(uniform_name, tmp);
}

void UniformManager::set_mat4x4_array(const unicode& uniform_name, const std::vector<Mat4>& matrices) {
    int32_t loc = locate(uniform_name);
    GLCheck(glUniformMatrix4fv, loc, matrices.size(), false, (GLfloat*) &matrices[0]);
}

void UniformManager::clear_uniform_cache() {
    uniform_cache_.clear();
}

void UniformManager::register_auto(ShaderAvailableAuto uniform, const unicode &var_name) {
    auto_uniforms_[uniform] = var_name;
}

//===================== END UNIFORMS =======================================

AttributeManager::AttributeManager(GPUProgram &program):
    program_(program) {}

int32_t AttributeManager::locate(const unicode& attribute) {
    if(!program_.is_complete()) {
        throw LogicError("Attempted to access attribute on a GPU program that is not complete");
    }

    auto it = attribute_cache_.find(attribute);
    if(it != attribute_cache_.end()) {
        return (*it).second;
    }

    GLint location = _GLCheck<GLint>(__func__, glGetAttribLocation, program_.program_object_, attribute.encode().c_str());
    if(location < 0) {
        L_ERROR(_u("Unable to find attribute with name {0}").format(attribute));
        throw LogicError(_u("Couldn't find attribute {0}").format(attribute).encode());
    }

    attribute_cache_[attribute] = location;

    return location;
}

void AttributeManager::set_location(const unicode& attribute, int32_t location) {
    //No completeness check, glBindAttribLocation can be called at any time

    GLCheck(glBindAttribLocation, program_.program_object_, location, attribute.encode().c_str());

    //Is this always true? Can we just assume that the location was given to that attribute?
    //The docs don't seem to suggest that it can fail...
    attribute_cache_[attribute] = location;
}

void AttributeManager::register_auto(ShaderAvailableAttributes attr, const unicode &var_name) {
    auto_attributes_[attr] = var_name;
}

//===================== END ATTRIBS ========================================


GPUProgram::GPUProgram():
    program_object_(0),
    uniforms_(*this),
    attributes_(*this) {}

const bool GPUProgram::is_current() const {
    GLint current = 0;
    GLCheck(glGetIntegerv, GL_CURRENT_PROGRAM, &current);
    return program_object_ && current == program_object_;
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
    L_DEBUG(_u("Created program {0}").format(program_object_));
}

bool GPUProgram::init() {
    return true;
}

void GPUProgram::cleanup()  {
    if(GLThreadCheck::is_current() && program_object_) {        
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
        throw LogicError("Unsupported shader type");
    }
}

void GPUProgram::set_shader_source(ShaderType type, const unicode& source) {
    if(source.empty()) {
        throw ValueError("Tried to set shader source to an empty string");
    }

    ShaderInfo new_shader;

    /*
        TERRIBLE TEMPORARY HACK, MAKE SHADER GLES COMPATIBLE
        The correct way to do this is to load different shader files
    */
#ifdef __ANDROID__
    int version_index = 0;
    auto lines = source.split("\n");
    for(uint32_t i = 0; i < lines.size(); ++i) {
        if(lines[i].starts_with("#version 120")) {
            lines[i] = "#version 100";
            version_index = i;
        }
    }

    lines.insert(lines.begin() + (version_index + 1), "precision mediump float;");
    new_shader.source = _u("\n").join(lines);
#else
    new_shader.source = source;
#endif

    is_linked_ = false; //We're no longer linked
    shaders_[type] = new_shader;
    shader_hashes_[type] = hashlib::MD5(source.encode()).hex_digest();
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

    std::string encoded_string = info.source.encode();
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
            L_ERROR("Unable to get the info log for the errornous shader, are you calling from the right thread?");
            throw RuntimeError("CRITICAL: Unable to get GLSL log");
        }

        std::vector<char> log;
        log.resize(length);

        GLCheck(glGetShaderInfoLog, info.object, length, (GLsizei*)NULL, &log[0]);
        std::string error_log(log.begin(), log.end());

        L_ERROR(error_log);
        throw RuntimeError("Unable to compile shader, check the error log for details");
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
        L_ERROR("Attempting to build a GPU program in the wrong thread");
    }

    prepare_program();

    for(auto p: shaders_) {
        compile(p.first); //Compile each shader if necessary
    }

    link(); //Now link the program
}

const bool GPUProgram::is_complete() const {
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

const bool GPUProgram::is_compiled(ShaderType type) const {
    return shaders_.at(type).is_compiled;
}

void GPUProgram::rebuild_hash() {
    hashlib::MD5 combined_hash;

    for(auto p: shader_hashes_) {
        combined_hash.update(p.second.encode());
    }

    md5_shader_hash_ = combined_hash.hex_digest();
}


void GPUProgram::link() {
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
        L_ERROR(error_log);

        throw RuntimeError("Couldn't link the GPU program. See error log for details.");
    }

    L_DEBUG(_u("Linked program {0}").format(program_object_));

    is_linked_ = true;
    signal_linked_();

    //DEBUG info!
    GLint count;
    glGetProgramiv(program_object_, GL_ACTIVE_UNIFORMS, &count);

    uniforms().uniform_info_.clear();

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

        uniforms().uniform_info_[info.name] = info;

        L_DEBUG(_u("UNIFORM {0} with size {1} and type {2}").format(std::string(buf, buf+buf_count), size, type));
    }
}

}
