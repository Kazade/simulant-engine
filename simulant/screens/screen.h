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

#ifndef SCREEN_H
#define SCREEN_H

/**
 *  Allows you to register different screens of gameplay, and
 *  easily switch between them.
 *
 *  manager->register_screen("/", screen_factory<LoadingScreen());
 *  manager->register_screen("/menu", screen_factory<MenuScreen());
 *  manager->register_screen("/ingame", screen_factory<GameScreen());
 *
 *  manager->activate_screen("/");
 *  manager->load_screen_in_background("/menu");
 *  if(manager->is_loaded("/menu")) {
 *      manager->activate_screen("/menu");
 *  }
 *  manager->unload("/");
 *  manager->activate_screen("/"); // Will cause loading to happen again
 *
 */

#include "../utils/unicode.h"
#include "../types.h"
#include "../window_base.h"
#include "../generic/managed.h"
#include "../generic/property.h"
#include "../interfaces/nameable.h"
#include "../interfaces.h"

namespace smlt {

class ScreenLoadException : public std::runtime_error {};

class ScreenBase : public Nameable {
public:
    typedef std::shared_ptr<ScreenBase> ptr;

    ScreenBase(WindowBase& window, const unicode& name);
    virtual ~ScreenBase();

    void load();
    void unload();

    void activate();
    void deactivate();

    void step(double dt);

    bool is_loaded() const { return is_loaded_; }

protected:
    Property<ScreenBase, WindowBase> window = { this, &ScreenBase::window_ };

    virtual void do_load() = 0;
    virtual void do_unload() {}
    virtual void do_activate() {}
    virtual void do_deactivate() {}
    virtual void do_step(double dt) {}

    PipelineID prepare_basic_scene(
        StageID& new_stage,
        CameraID& new_camera,
        AvailablePartitioner partitioner=PARTITIONER_HASH
    );

    WindowBase* window_;

private:
    bool is_loaded_ = false;
};

template<typename T>
class Screen : public ScreenBase, public Managed<T> {
public:
    Screen(WindowBase& window, const unicode& name):
        ScreenBase(window, name) {}

    void cleanup() override {
        do_unload();
    }
};

}


#endif // SCREEN_H
