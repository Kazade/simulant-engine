
#ifndef __ANDROID__
	#include <GL/glew.h>
#else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>

    #define GL_VERTEX_ARRAY_BINDING GL_VERTEX_ARRAY_BINDING_OES

    //Defined in buffer_object.cpp
    #define glGenVertexArrays glGenVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES
#endif
