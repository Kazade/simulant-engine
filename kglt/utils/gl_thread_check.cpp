#include <kazbase/unicode.h>

#include "gl_thread_check.h"

std::shared_ptr<GLThreadCheck> GL_thread;

void GLThreadCheck::check() {
    try {
        GL_thread->do_check();
    } catch(WrongThreadError& e) {
        L_ERROR("Tried to call OpenGL dependent code from the wrong thread");
        throw;
    }
}
