#include <chrono>
#include <future>

#include "window.h"
#include "application.h"
#include "screens/loading.h"
#include "input_controller.h"

namespace kglt {

Application::Application(const unicode &title, uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen) {
    window_ = Window::create(width, height, bpp, fullscreen);

    window_->set_title(title.encode());

    window_->signal_step().connect(std::bind(&Application::do_step, this, std::placeholders::_1));
    window_->signal_post_step().connect(std::bind(&Application::do_post_step, this, std::placeholders::_1));
    window_->signal_shutdown().connect(std::bind(&Application::do_cleanup, this));

    window_->keyboard().key_pressed_connect(std::bind(&Application::on_key_press, this, std::placeholders::_1));
    window_->keyboard().key_released_connect(std::bind(&Application::on_key_release, this, std::placeholders::_1));
    window_->keyboard().key_while_pressed_connect(std::bind(&Application::while_key_pressed, this, std::placeholders::_1, std::placeholders::_2));
}

StagePtr Application::stage(StageID stage) {
    return window->stage(stage);
}

bool Application::init() {
    // Add some useful screens by default, these can be overridden in do_init if the
    // user so wishes
    register_screen("/loading", screen_factory<screens::Loading>());

    initialized_ = do_init();

    // If we successfully initialized, but the user didn't specify
    // a particular screen, we just hit the root route
    if(initialized_ && !active_screen()) {
        activate_screen("/");
    }

    return initialized_;
}


int32_t Application::run() {
    if(!init()) {
        L_ERROR("Error while initializing, terminating application");
        return 1;
    }

    while(window_->run_frame()) {}

    return 0;
}

}
