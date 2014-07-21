#include "window.h"
#include "application.h"
#include "screens/loading.h"
#include "input_controller.h"

namespace kglt {

Application::Application(const unicode &title, uint32_t width, uint32_t height, uint32_t bpp, bool fullscreen) {
    window_ = Window::create(width, height, bpp, fullscreen);
    window_->set_title(title.encode());    

    window_->signal_frame_started().connect(std::bind(&Application::check_tasks, this));
    window_->signal_step().connect(std::bind(&Application::do_step, this, std::placeholders::_1));
    window_->signal_post_step().connect(std::bind(&Application::do_post_step, this, std::placeholders::_1));
    window_->signal_shutdown().connect(std::bind(&Application::do_cleanup, this));

    window_->keyboard().key_pressed_connect(std::bind(&Application::on_key_press, this, std::placeholders::_1));
    window_->keyboard().key_released_connect(std::bind(&Application::on_key_release, this, std::placeholders::_1));
    window_->keyboard().key_while_pressed_connect(std::bind(&Application::while_key_pressed, this, std::placeholders::_1, std::placeholders::_2));
}

StagePtr Application::stage(StageID stage) {
    return window().stage(stage);
}

void Application::load_async(boost::function<bool ()> func) {
    //Start the task loading in the background
    boost::shared_future<bool> future = boost::async(boost::launch::async, func);
    load_tasks_.push_back(future);

    window_->loading().activate(); //Activate the loading screen
}

void Application::check_tasks() {
    //Run any background loading tasks
    for(auto it = load_tasks_.begin(); it != load_tasks_.end(); ++it) {
        auto result = (*it).wait_for(boost::chrono::seconds(0));
        if(result == boost::future_status::ready) {
            if(!(*it).get()) {
                throw BackgroundLoadException();
            }
            it = load_tasks_.erase(it);
            if(load_tasks_.empty()) {
                window_->loading().deactivate();
            }
        }
    }
}

int32_t Application::run() {
    load_async(std::bind(&Application::init, this));

    while(window_->run_frame()) {}

    return 0;
}

}
