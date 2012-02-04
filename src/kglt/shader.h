#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <string>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#include "glee/GLee.h"

#include "kazmath/mat4.h"
#include "kazmath/mat3.h"
#include "kazmath/vec3.h"

#include "loadable.h"

namespace kglt {

const std::string default_vert_shader_120 = R"(
#version 120

uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

void main() {
    gl_Position = modelview_projection_matrix * gl_Vertex;
    gl_TexCoord[0]  = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}

)";

const std::string default_frag_shader_120 = R"(
#version 120

uniform sampler2D texture_1;

void main() {
    gl_FragColor = texture2D(texture_1, gl_TexCoord[0].st);
}

)";

enum ShaderType {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_MAX
};

class ShaderProgram : public Loadable {
public:
    ShaderProgram();
    ~ShaderProgram();

    void activate();
    void add_and_compile(ShaderType type, const std::string& source);

    void set_uniform(const std::string& name, const float x);
    void set_uniform(const std::string& name, const int32_t x);
    void set_uniform(const std::string& name, const kmMat4* matrix);
    void set_uniform(const std::string& name, const kmMat3* matrix);
    void set_uniform(const std::string& name, const kmVec3* vec);

    void bind_attrib(uint32_t idx, const std::string& name);
    
    bool has_uniform(const std::string& name);    
private:
    GLint get_uniform_loc(const std::string& name);

    GLuint program_id_;
    GLuint shader_ids_[SHADER_TYPE_MAX];

    std::map<std::string, GLint> cached_uniform_locations_;
};

}

#endif // SHADER_H_INCLUDED
