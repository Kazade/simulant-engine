#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <type_traits>
#include <thread>
#include <kazbase/exceptions.h>
#include "../generic/property.h"
#include "../generic/managed.h"

namespace kglt {

class Material;
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

    void pre_fixed_update(double step) {
        do_pre_fixed_update(step);
    }

    void fixed_update(double step) {
        do_fixed_update(step);
    }

    void post_fixed_update(double step) {
        do_post_fixed_update(step);
    }

    Property<Controller, std::string> name = { this, &Controller::name_ };
private:
    virtual void do_pre_update(double dt) {}
    virtual void do_update(double dt) {}
    virtual void do_post_update(double dt) {}

    virtual void do_pre_fixed_update(double step) {}
    virtual void do_fixed_update(double step) {}
    virtual void do_post_fixed_update(double step) {}

    std::string name_;
};

class MaterialController : public Controller {
public:
    MaterialController(const std::string& name, Material* material):
        Controller(name),
        material_(material) {
    }

protected:
    Property<MaterialController, Material> material = { this, &MaterialController::material_ };

private:
    Material* material_;
};

class Controllable {
public:
    virtual ~Controllable() {}

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
    std::shared_ptr<T> new_controller() {
        static_assert(std::is_base_of<Controller, T>::value, "Controllers must derive kglt::Controller");
        static_assert(std::is_base_of<Managed<T>, T>::value, "Controllers must derive Managed<T>");
        std::shared_ptr<T> ret = T::create(this);
        add_controller(ret);
        return ret;
    }

    template<typename T, typename ...Params>
    std::shared_ptr<T> new_controller(Params&&... params) {
        static_assert(std::is_base_of<Controller, T>::value, "Controllers must derive kglt::Controller");
        static_assert(std::is_base_of<Managed<T>, T>::value, "Controllers must derive Managed<T>");
        std::shared_ptr<T> ret = T::create(this, std::forward<Params>(params)...);
        add_controller(ret);
        return ret;
    }

    void pre_fixed_update_controllers(double step) {
        for(auto& controller: controllers_) {
            controller->pre_fixed_update(step);
        }
    }

    void fixed_update_controllers(double step) {
        for(auto& controller: controllers_) {
            controller->fixed_update(step);
        }
    }

    void post_fixed_update_controllers(double step) {
        for(auto& controller: controllers_) {
            controller->post_fixed_update(step);
        }
    }

    void update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->update(dt);
        }
    }

    void pre_update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->pre_update(dt);
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
