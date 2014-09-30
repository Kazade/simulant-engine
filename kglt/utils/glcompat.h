
#ifndef __ANDROID__
	#include <GL/glew.h>
#else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

    #define GL_VERTEX_ARRAY_BINDING GL_VERTEX_ARRAY_BINDING_OES

    #define GL_STREAM_READ 0x88E1
    #define GL_STREAM_COPY 0x88E2
    #define GL_STATIC_READ 0x88E5
    #define GL_STATIC_COPY 0x88E6
    #define GL_DYNAMIC_READ 0x88E9
    #define GL_DYNAMIC_COPY 0x88EA

    //Defined in buffer_object.cpp
    extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays;
    extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray;
    extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArrays;
#endif
