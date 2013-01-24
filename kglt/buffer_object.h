#ifndef BUFFER_OBJECT_H
#define BUFFER_OBJECT_H

#include <GLee.h>

namespace kglt {

enum BufferObjectType {
    BUFFER_OBJECT_VERTEX_DATA,
    BUFFER_OBJECT_INDEX_DATA
};

enum BufferObjectUsage {
    MODIFY_ONCE_USED_FOR_LIMITED_RENDERING,
    MODIFY_ONCE_USED_FOR_LIMITED_QUERYING,
    MODIFY_ONCE_USED_FOR_LIMITED_QUERYING_AND_RENDERING,
    MODIFY_ONCE_USED_FOR_RENDERING,
    MODIFY_ONCE_USED_FOR_QUERYING,
    MODIFY_ONCE_USED_FOR_QUERYING_AND_RENDERING,
    MODIFY_REPEATEDLY_USED_FOR_RENDERING,
    MODIFY_REPEATEDLY_USED_FOR_QUERYING,
    MODIFY_REPEATEDLY_USED_FOR_QUERYING_AND_RENDERING
};

class BufferObject {
public:
    BufferObject(BufferObjectType type, BufferObjectUsage usage=MODIFY_ONCE_USED_FOR_RENDERING);
    ~BufferObject();

    void bind() const;
    void create(uint32_t byte_size, const void* data);
    void modify(uint32_t offset, uint32_t byte_size, const void* data);

private:
    BufferObjectUsage usage_;

    GLenum gl_target_;
    GLuint buffer_id_;
    bool initialized_;
};

}

#endif // BUFFER_OBJECT_H
