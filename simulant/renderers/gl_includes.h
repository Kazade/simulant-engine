#pragma once

#ifdef __DREAMCAST__
    #include "../../../deps/libgl/include/GL/gl.h"
#elif defined(__PSP__)
    #include <GL/gl.h>

    /* PSP doesn't include these constants, we don't use them but need them for
     * compilation */

    #define GL_PALETTE4_RGB8_OES              0x8B90
    #define GL_PALETTE4_RGBA8_OES             0x8B91
    #define GL_PALETTE4_R5_G6_B5_OES          0x8B92
    #define GL_PALETTE4_RGBA4_OES             0x8B93
    #define GL_PALETTE4_RGB5_A1_OES           0x8B94
    #define GL_PALETTE8_RGB8_OES              0x8B95
    #define GL_PALETTE8_RGBA8_OES             0x8B96
    #define GL_PALETTE8_R5_G6_B5_OES          0x8B97
    #define GL_PALETTE8_RGBA4_OES             0x8B98
    #define GL_PALETTE8_RGB5_A1_OES           0x8B99

#elif defined(__XBOX__)
    #include <pbgl.h>
    #include <hal/debug.h>
    #include <GL/gl.h>
    #define GL_GLEXT_PROTOTYPES
    #include <GL/glext.h>

    #define glGenerateMipmapEXT glGenerateMipmap
    #define GL_PALETTE4_RGB8_OES              0x8B90
    #define GL_PALETTE4_RGBA8_OES             0x8B91
    #define GL_PALETTE4_R5_G6_B5_OES          0x8B92
    #define GL_PALETTE4_RGBA4_OES             0x8B93
    #define GL_PALETTE4_RGB5_A1_OES           0x8B94
    #define GL_PALETTE8_RGB8_OES              0x8B95
    #define GL_PALETTE8_RGBA8_OES             0x8B96
    #define GL_PALETTE8_R5_G6_B5_OES          0x8B97
    #define GL_PALETTE8_RGBA4_OES             0x8B98
    #define GL_PALETTE8_RGB5_A1_OES           0x8B99

    #define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

    GL_API static inline void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
        debugPrint("glCompressedTexImage2D unimplemented");
    }
#elif defined(SIMULANT_USE_GLAD)
    #include "./glad/glad/glad.h"
#endif
