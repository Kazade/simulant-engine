#include <kazbase/unicode.h>

#include "gl_thread_check.h"

std::shared_ptr<GLThreadCheck> GL_thread;

void GLThreadCheck::check(const unicode& message) {
    try {
        GL_thread->do_check();
    } catch(WrongThreadError& e) {
        L_ERROR("Tried to call OpenGL dependent code from the wrong thread");
        L_ERROR(_u("Message was: {0}").format(message));
        throw;
    }
}
