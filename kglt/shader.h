#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include <string>
#include <tr1/memory>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#include "kazmath/mat4.h"
#include "kazmath/mat3.h"
#include "kazmath/vec3.h"

#include "loadable.h"
#include "generic/identifiable.h"
#include "types.h"

namespace kglt {

class Scene;

enum ShaderType {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_MAX
};

class ShaderProgram :
    public Loadable,
    public generic::Identifiable<ShaderID> {

public:
	typedef std::tr1::shared_ptr<ShaderProgram> ptr;

    ShaderProgram(Scene* scene, ShaderID id);
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
    
    void relink();
private:
	ShaderProgram(const ShaderProgram& rhs);
	ShaderProgram& operator=(const ShaderProgram& rhs);

    int32_t get_uniform_loc(const std::string& name);

    uint32_t program_id_;
    uint32_t shader_ids_[SHADER_TYPE_MAX];

    std::map<std::string, int32_t> cached_uniform_locations_;    
};

}

#endif // SHADER_H_INCLUDED
