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

#ifndef behaviour_H
#define behaviour_H

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
#include "../interfaces/updateable.h"
#include "../input/input_manager.h"

namespace smlt {

class InputManager;
class Material;
class Behaviour;
class Organism;

typedef std::shared_ptr<Behaviour> BehaviourPtr;

class Behaviour:
    public Updateable {
public:
    virtual ~Behaviour() {}

    virtual const std::string name() const = 0;

    void enable();
    void disable();

    void _update_thunk(float dt) override;
    void _late_update_thunk(float dt) override;
    void _fixed_update_thunk(float step) override;

private:
    friend class Organism;

    virtual void on_behaviour_added(Organism* controllable) {}
    virtual void on_behaviour_removed(Organism* controllable) {}

    /* Called on the first update after being enabled */
    virtual void on_behaviour_first_update(Organism* controllable) {}

    bool is_enabled_ = true;
    bool first_update_done_ = false;
};

class BehaviourWithInput : public Behaviour {
public:
    BehaviourWithInput(InputManager* input):
        input_(input) {}

protected:
    Property<BehaviourWithInput, InputManager> input = {this, &BehaviourWithInput::input_};

private:
    InputManager* input_;
};

class MaterialBehaviour : public Behaviour {
public:
    MaterialBehaviour(Material* material):
        Behaviour(),
        material_(material) {
    }

protected:
    Property<MaterialBehaviour, Material> material = { this, &MaterialBehaviour::material_ };
    Material* get_material() const { return material_; }

private:
    Material* material_;
};

class Organism {
public:
    virtual ~Organism() {
        behaviours_.clear();
        behaviour_names_.clear();
        behaviour_types_.clear();
    }

    template<typename T>
    bool has_behaviour() const {
        return behaviour_types_.count(typeid(T).hash_code()) > 0;
    }

    template<typename T>
    T* behaviour() const {
        auto hash_code = typeid(T).hash_code();
        if(!behaviour_types_.count(hash_code)) {
            return nullptr;
        }

        return dynamic_cast<T*>(behaviour_types_.at(hash_code).get());
    }

    void add_behaviour(BehaviourPtr behaviour) {
        if(!behaviour) {
            L_WARN("Tried to add a null behaviour to the controllable");
            return;
        }

        std::lock_guard<std::mutex> lock(container_lock_);

        if(behaviour_names_.count(behaviour->name())) {
            L_WARN(_F("Tried to add a duplicate behaviour: {0}").format((std::string)behaviour->name()));
            return;
        }

        behaviour_types_.insert(std::make_pair(typeid(*behaviour).hash_code(), behaviour));
        behaviour_names_.insert(behaviour->name());
        behaviours_.push_back(behaviour);

        behaviour->on_behaviour_added(this);
    }

    template<typename T>
    T* new_behaviour() {
        static_assert(std::is_base_of<Behaviour, T>::value, "Behaviours must derive smlt::Behaviour");
        static_assert(std::is_base_of<Managed<T>, T>::value, "Behaviours must derive Managed<T>");
        std::shared_ptr<T> ret = T::create(this);
        add_behaviour(ret);
        return ret.get();
    }

    template<typename T, typename ...Params>
    T* new_behaviour(Params&&... params) {
        static_assert(std::is_base_of<Behaviour, T>::value, "Behaviours must derive smlt::Behaviour");
        static_assert(std::is_base_of<Managed<T>, T>::value, "Behaviours must derive Managed<T>");
        std::shared_ptr<T> ret = T::create(this, std::forward<Params>(params)...);
        add_behaviour(ret);
        return ret.get();
    }

    void fixed_update_behaviours(float step) {
        for(auto& behaviour: behaviours_) {
            behaviour->_fixed_update_thunk(step);
        }
    }

    void update_behaviours(float dt) {
        for(auto& behaviour: behaviours_) {

            // Call any overridden functions looking for first update
            if(!behaviour->first_update_done_) {
                behaviour->on_behaviour_first_update(this);
                behaviour->first_update_done_ = true;
            }

            behaviour->_update_thunk(dt);
        }
    }

    void late_update_behaviours(float dt) {
        for(auto& behaviour: behaviours_) {
            behaviour->_late_update_thunk(dt);
        }
    }

private:
    std::mutex container_lock_;

    std::vector<BehaviourPtr> behaviours_;
    std::unordered_set<std::string> behaviour_names_;
    std::unordered_map<std::size_t, BehaviourPtr> behaviour_types_;

};


}
#endif // behaviour_H
