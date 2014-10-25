/*
 *  This is a little crazy...
 *
 *  Basically, OpenGL 3.1 (which is what KGLT targets) requires that you use VAOs for rendering,
 *  however, GLES 2.0 (which is what we have to target on Android) doesn't support VAOs, except for
 *  via a flaky extension.
 *
 *  This code simulates VAOs in the case that they aren't supported. It does this by wrapping calls
 *  to glVertexAttribPointer and storing the bound buffers, and the vertex pointer calls. Then,
 *  when you bind the vertex array object, these vertex attrib pointer calls are replayed.
 *
 *  It's slow and hacky, but it seems to work. I'm not totally happy with it, but I don't want to scatter
 *  a load of #ifdefs through my code either. In time this will go away once GLES 3.0 support is more
 *  widely available and we can use VAOs everywhere
 */

#include <unordered_map>
#include <functional>
#include <memory>

#include <dlfcn.h>

#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include <kazbase/exceptions.h>

#include "vao_abstraction.h"

PFNGLGENVERTEXARRAYSPROC vaoGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC vaoBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC vaoDeleteVertexArrays;
PFNGLISVERTEXARRAYPROC vaoIsVertexArray;
PFNGLVERTEXATTRIBPOINTERPROC vaoVertexAttribPointer;

GETINTEGERV vaoGetIntegerv;

static GLuint vao_id = 0;
static GLuint currently_bound = 0;

struct ClientState {
    GLint bound_buffer;
    GLboolean is_enabled;
    std::function<void ()> attrib_pointer_call;
};

struct VAOState {
    GLuint id;
    GLint element_array_buffer = 0;
    std::unordered_map<GLuint, ClientState> client_state_by_index;
};

std::unordered_map<GLuint, std::shared_ptr<VAOState>> array_objects;

void get_integer_v(GLenum pname, GLint* params) {
    if(pname != GL_VERTEX_ARRAY_BINDING) {
        throw ValueError("Only use this function to get the vertex array binding");
    }

    params[0] = currently_bound;
}

void gen_vertex_arrays(GLsizei n, GLuint* arrays) {
    for(GLsizei i = 0; i < n; ++i) {
        arrays[i] = ++vao_id;
        array_objects[arrays[i]] = std::make_shared<VAOState>();
    }
}

void bind_vertex_array(GLuint array) {
    if(array == currently_bound) {
        //There's really no point doing anything if it's already bound
        return;
    }

    if(!array) {
        currently_bound = 0;
        return;
    }

    auto state = array_objects.at(array);

    if(state->element_array_buffer) {
        //But the element array buffer that was part of the VAO state
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->element_array_buffer);

        //Bind the array buffers and call the stashed vertex pointer calls
        for(auto p: state->client_state_by_index) {
            if(!p.second.is_enabled) continue;

            glBindBuffer(GL_ARRAY_BUFFER, p.second.bound_buffer); //Bind whatever array buffer was bound when the pointer call was made
            p.second.attrib_pointer_call(); //Make the pointer call again
        }
    }

    currently_bound = array;
}

void delete_vertex_arrays(GLsizei n, const GLuint* arrays) {
    for(GLsizei i = 0; i < n; ++i) {
        array_objects.erase(arrays[i]);
    }
}

GLboolean is_vertex_array(GLuint array) {
    return array_objects.find(array) != array_objects.end();
}

void vertex_attrib_pointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
    /*
     * Here we store the call that was made, and store the current state of the bound buffers. Then, when the VAO is bound
     * we can rebind the appropriate buffers and replay the AttribPointer calls
     */

    std::function<void ()> to_stash = std::bind(glVertexAttribPointer, index, size, type, normalized, stride, pointer);

    auto& state = array_objects[currently_bound]->client_state_by_index[index];
    state.attrib_pointer_call = to_stash;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.bound_buffer);

    GLint enabled;
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    state.is_enabled = (GLboolean) enabled;

    //Hacky but we don't want to override any other functions (e.g. glBindBuffer)
    if(array_objects[currently_bound]->element_array_buffer == 0) {
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,  &array_objects[currently_bound]->element_array_buffer);
    }

    //Call through to glVertexAttribPointer anyway, we've stored what we need to bind when we call bind!
    to_stash();
}

void vao_init() {
    int major_version = 0;

    unicode extensions = (char*) glGetString(GL_EXTENSIONS);
    if(extensions.contains("GL_OES_vertex_array_object")) {
        //Let's connect up to the OES version of the functions
        void *libhandle = dlopen("libGLESv2.so", RTLD_LAZY);
        vaoGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) dlsym(libhandle, "glGenVertexArraysOES");
        vaoBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) dlsym(libhandle, "glBindVertexArrayOES");
        vaoDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) dlsym(libhandle, "glDeleteVertexArraysOES");
        vaoIsVertexArray = (PFNGLISVERTEXARRAYPROC)dlsym(libhandle, "glIsVertexArrayOES");
        vaoVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) dlsym(libhandle, "glVertexAttribPointer");
        vaoGetIntegerv = (GETINTEGERV) dlsym(libhandle, "glGetIntegerv");
    } else {
        unicode version = (char*) glGetString(GL_VERSION);
        L_INFO(_u("OpenGL version: {0}").format(version));

        bool is_es = version.starts_with("OpenGL ES");

        version = version.replace("OpenGL ES", "").strip(); //Annoyingly, version strings on Android don't just start with a number :/
        major_version = version.split(".")[0].to_int();

        if(!is_es && (major_version >= 3 || extensions.contains("GL_ARB_vertex_array_object"))) {
            void *libhandle = dlopen("libGL.so", RTLD_LAZY);
            //Part of core or an extension that GLEW would have loaded, just assign the things as normal
            vaoGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) dlsym(libhandle, "glGenVertexArrays");
            vaoBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) dlsym(libhandle, "glBindVertexArray");
            vaoDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) dlsym(libhandle, "glDeleteVertexArrays");
            vaoIsVertexArray = (PFNGLISVERTEXARRAYPROC)dlsym(libhandle, "glIsVertexArray");
            vaoVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) dlsym(libhandle, "glVertexAttribPointer");
            vaoGetIntegerv = (GETINTEGERV) dlsym(libhandle, "glGetIntegerv");
        } else {
            L_INFO("Using VAO abstraction layer as they are unsupported");
            //Connect our simulation of VAOs!
            vaoGenVertexArrays = gen_vertex_arrays;
            vaoBindVertexArray = bind_vertex_array;
            vaoDeleteVertexArrays = delete_vertex_arrays;
            vaoIsVertexArray = is_vertex_array;
            vaoVertexAttribPointer = vertex_attrib_pointer;
            vaoGetIntegerv = get_integer_v;
        }
    }
}
