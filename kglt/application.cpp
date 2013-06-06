#include "window.h"
#include "scene.h"
#include "application.h"
#include "screens/loading.h"

namespace kglt {

App::App(const unicode &title) {
    window_ = Window::create();
    window_->set_title(title.encode());

    window_->signal_frame_started().connect(std::bind(&App::check_tasks, this));
    window_->signal_step().connect(std::bind(&App::do_step, this, std::placeholders::_1));
    window_->signal_shutdown().connect(std::bind(&App::do_cleanup, this));
}

Scene& App::scene() {
    return window().scene();
}

Stage& App::stage(StageID stage) {
    return window().scene().stage(stage);
}

void App::load_async(std::function<bool ()> func) {
    //Start the task loading in the background
    std::shared_future<bool> future = std::async(std::launch::async, func);
    load_tasks_.push_back(future);

    window_->loading().activate(); //Activate the loading screen
}

void App::check_tasks() {
    //Run any background loading tasks
    for(auto it = load_tasks_.begin(); it != load_tasks_.end(); ++it) {
        if((*it).wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
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

int32_t App::run() {
    load_async(std::bind(&App::do_init, this));

    while(window_->update()) {}

    return 0;
}

}
