#ifndef GL_THREAD_CHECK_H
#define GL_THREAD_CHECK_H

#include <stdexcept>
#include <thread>
#include <memory>

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

    static void check() {
        GL_thread->do_check();
    }

private:
    GLThreadCheck(std::thread::id render_thread):
        render_thread_id_(render_thread) {}

    void do_check() {

        if(std::this_thread::get_id() != render_thread_id_) {
            throw WrongThreadError();
        }
    }


    std::thread::id render_thread_id_;
};

#endif // GL_THREAD_CHECK_H
