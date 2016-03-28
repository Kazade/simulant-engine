#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>
#include <set>
#include <memory>
#include <type_traits>
#include <thread>
#include <kazbase/exceptions.h>
#include "../generic/property.h"

namespace kglt {

class Controller;

typedef std::shared_ptr<Controller> ControllerPtr;

class Controller {
public:
    Controller(const std::string& name):
        name_(name) {}

    virtual ~Controller() {}

    void pre_update(double dt) {
        do_pre_update(dt);
    }

    void update(double dt) {
        do_update(dt);
    }

    void post_update(double dt) {
        do_post_update(dt);
    }

    Property<Controller, std::string> name = { this, &Controller::name_ };
private:
    virtual void do_pre_update(double dt) {}
    virtual void do_update(double dt) {}
    virtual void do_post_update(double dt) {}

    std::string name_;
};

class ControllerContainer {
public:
    virtual ~ControllerContainer() {}

    void add_controller(ControllerPtr controller) {
        if(!controller) {
            throw LogicError("Tried to add null controller");
        }

        std::lock_guard<std::mutex> lock(container_lock_);

        if(controller_names_.count(controller->name)) {
            throw LogicError(_u("Tried to add a duplicate controller: {0}").format((std::string)controller->name));
        }

        controller_names_.insert(controller->name);
        controllers_.push_back(controller);
    }

    template<typename T>
    ControllerPtr new_controller() {
        static_assert(std::is_base_of<Controller, T>::value, "Controllers must derive kglt::Controller");
        ControllerPtr ret = std::make_shared<T>(this);
        add_controller(ret);
        return ret;
    }

    template<typename T, typename ...Params>
    ControllerPtr new_controller(Params&&... params) {
        static_assert(std::is_base_of<Controller, T>::value, "Controllers must derive kglt::Controller");
        ControllerPtr ret = std::make_shared<T>(this, std::forward<Params>(params)...);
        add_controller(ret);
        return ret;
    }

protected:
    void pre_update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->pre_update(dt);
        }
    }

    void update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->update(dt);
        }
    }

    void post_update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->post_update(dt);
        }
    }

private:
    std::mutex container_lock_;

    std::vector<ControllerPtr> controllers_;
    std::set<std::string> controller_names_;

};


}
#endif // CONTROLLER_H
