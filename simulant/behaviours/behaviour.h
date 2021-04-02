/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef behaviour_H
#define behaviour_H

#include <vector>
#include <set>
#include <memory>
#include <type_traits>

#include "../logging.h"

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

    virtual const char* name() const = 0;

    void enable();
    void disable();

    void _update_thunk(float dt) override;
    void _late_update_thunk(float dt) override;
    void _fixed_update_thunk(float step) override;

    Property<Organism* Behaviour::*> organism = {
        this, &Behaviour::organism_
    };

    bool attached() const { return organism_ != nullptr; }
private:
    friend class Organism;

    Organism* organism_ = nullptr;
    void set_organism(Organism* organism) {
        if(organism) {
            if(organism_) {
                on_behaviour_removed(organism_);
            }

            organism_ = organism;

            on_behaviour_added(organism);
        } else if(organism_) {
            organism_ = nullptr;

            on_behaviour_removed(organism_);
        }
    }

    /* Don't call these directly, use set_organism instead */
    virtual void on_behaviour_added(Organism* controllable) {
        _S_UNUSED(controllable);
    }

    virtual void on_behaviour_removed(Organism* controllable) {
        _S_UNUSED(controllable);
    }

    /* Called on the first update after being enabled */
    virtual void on_behaviour_first_update(Organism* controllable) {
        _S_UNUSED(controllable);
    }

    bool is_enabled_ = true;
    bool first_update_done_ = false;
};

class BehaviourWithInput : public Behaviour {
public:
    BehaviourWithInput(InputManager* input):
        input_(input) {}

protected:
    Property<InputManager* BehaviourWithInput::*> input = {
        this, &BehaviourWithInput::input_
    };

private:
    InputManager* input_;
};

class MaterialBehaviour : public Behaviour {
public:
    MaterialBehaviour(Material* material):
        Behaviour(),
        material_(material) {
    }

private:
    Material* material_;

protected:
    Property<decltype(&MaterialBehaviour::material_)> material = {
        this, &MaterialBehaviour::material_
    };
    Material* get_material() const { return material_; }

};

class Organism {
public:
    virtual ~Organism();

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

    template<typename T>
    T* new_behaviour() {
        static_assert(std::is_base_of<Behaviour, T>::value, "Behaviours must derive smlt::Behaviour");
        static_assert(std::is_base_of<RefCounted<T>, T>::value, "Behaviours must derive RefCounted<T>");
        std::shared_ptr<T> ret = T::create();
        add_behaviour<T>(ret);
        return ret.get();
    }

    template<typename T, typename ...Params>
    T* new_behaviour(Params&&... params) {
        static_assert(std::is_base_of<Behaviour, T>::value, "Behaviours must derive smlt::Behaviour");
        static_assert(std::is_base_of<RefCounted<T>, T>::value, "Behaviours must derive RefCounted<T>");
        std::shared_ptr<T> ret = T::create(std::forward<Params>(params)...);
        add_behaviour<T>(ret);
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
    template<typename T>
    void add_behaviour(std::shared_ptr<T> behaviour) {
        static_assert(std::is_base_of<Behaviour, T>::value, "Behaviours must derive smlt::Behaviour");

        if(!behaviour) {
            S_WARN("Tried to add a null behaviour to the controllable");
            return;
        }

        {
            if(behaviour_names_.count(behaviour->name())) {
                S_WARN("Tried to add a duplicate behaviour: {0}", (std::string)behaviour->name());
                return;
            }

            behaviour_types_.insert(std::make_pair(typeid(T).hash_code(), behaviour));
            behaviour_names_.insert(behaviour->name());
            behaviours_.push_back(behaviour);
        }

        // Call outside the lock to prevent deadlocking if
        // this call triggers the addition/removal of another behaviour
        behaviour->set_organism(this);
    }

    std::vector<BehaviourPtr> behaviours_;
    std::unordered_set<std::string> behaviour_names_;
    std::unordered_map<std::size_t, BehaviourPtr> behaviour_types_;

};


}
#endif // behaviour_H
