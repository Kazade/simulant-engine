#ifndef GL_THREAD_CHECK_H
#define GL_THREAD_CHECK_H

#include <stdexcept>
#include <thread>
#include <memory>

#include "../deps/kazlog/kazlog.h"

class WrongThreadError:
    public std::runtime_error {

public:
    WrongThreadError():
        std::runtime_error("Attempted to call OpenGL from the wrong thread") {}
};

class GLThreadCheck;

extern std::shared_ptr<GLThreadCheck> GL_thread;

class GLThreadCheck {
public:
    static void init() {
        GL_thread.reset(new GLThreadCheck(std::this_thread::get_id()));
    }

    static void cleanup() {
        GL_thread.reset();
    }

    static void check();

    static bool is_current() {
        if(!GL_thread) {
            return false;
        }

        return GL_thread->do_check(false);
    }

private:
    GLThreadCheck(std::thread::id render_thread):
        render_thread_id_(render_thread) {}

    bool do_check(bool raise=true) {

        if(std::this_thread::get_id() != render_thread_id_) {
            if(raise) {
                throw WrongThreadError();
            } else {
                return false;
            }
        }

        return true;
    }


    std::thread::id render_thread_id_;
};

#endif // GL_THREAD_CHECK_H
