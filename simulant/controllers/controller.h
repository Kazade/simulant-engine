/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <type_traits>
#include <thread>

#include "../deps/kazlog/kazlog.h"

#include "../generic/property.h"
#include "../generic/managed.h"

namespace smlt {

class Material;
class Controller;

typedef std::shared_ptr<Controller> ControllerPtr;

class Controller {
public:
    Controller(const std::string& name):
        name_(name) {}

    virtual ~Controller() {}

    void enable();
    void disable();

    void update(double dt);
    void late_update(double dt);
    void fixed_update(double step);

    Property<Controller, std::string> name = { this, &Controller::name_ };

private:
    virtual void do_update(double dt) {}
    virtual void do_late_update(double dt) {}
    virtual void do_fixed_update(double step) {}

    std::string name_;
    bool is_enabled_ = true;
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
            L_WARN("Tried to add a null controller to the controllable");
            return;
        }

        std::lock_guard<std::mutex> lock(container_lock_);

        if(controller_names_.count(controller->name)) {
            L_WARN(_F("Tried to add a duplicate controller: {0}").format((std::string)controller->name));
            return;
        }

        controller_names_.insert(controller->name);
        controllers_.push_back(controller);
    }

    template<typename T>
    std::shared_ptr<T> new_controller() {
        static_assert(std::is_base_of<Controller, T>::value, "Controllers must derive smlt::Controller");
        static_assert(std::is_base_of<Managed<T>, T>::value, "Controllers must derive Managed<T>");
        std::shared_ptr<T> ret = T::create(this);
        add_controller(ret);
        return ret;
    }

    template<typename T, typename ...Params>
    std::shared_ptr<T> new_controller(Params&&... params) {
        static_assert(std::is_base_of<Controller, T>::value, "Controllers must derive smlt::Controller");
        static_assert(std::is_base_of<Managed<T>, T>::value, "Controllers must derive Managed<T>");
        std::shared_ptr<T> ret = T::create(this, std::forward<Params>(params)...);
        add_controller(ret);
        return ret;
    }

    void fixed_update_controllers(double step) {
        for(auto& controller: controllers_) {
            controller->fixed_update(step);
        }
    }

    void update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->update(dt);
        }
    }

    void late_update_controllers(double dt) {
        for(auto& controller: controllers_) {
            controller->late_update(dt);
        }
    }

private:
    std::mutex container_lock_;

    std::vector<ControllerPtr> controllers_;
    std::set<std::string> controller_names_;

};


}
#endif // CONTROLLER_H
