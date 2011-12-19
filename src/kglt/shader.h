#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <string>

#include "GL/glew.h"

#include "kazmath/mat4.h"
#include "kazmath/mat3.h"
#include "kazmath/vec3.h"

#include "loadable.h"

namespace kglt {

enum ShaderType {
    VERTEX,
    FRAGMENT,
    MAX_SHADER_TYPES
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

private:
    GLint get_uniform_loc(const std::string& name);

    GLuint program_id_;
    GLuint shader_ids_[MAX_SHADER_TYPES];

};

}

#endif // SHADER_H_INCLUDED
