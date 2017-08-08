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

#ifndef SCENE_H
#define SCENE_H

/**
 *  Allows you to register different scenes of gameplay, and
 *  easily switch between them.
 *
 *  manager->register_scene<LoadingScene>("loading");
 *  manager->register_scene<MenuScene>("menu");
 *  manager->register_scene<GameScene>("ingame");
 *
 *  manager->activate_scene("loading");
 *  manager->load_scene_in_background("menu");
 *  if(manager->is_loaded("menu")) {
 *      manager->activate_scene("menu");
 *  }
 *  manager->unload("loading");
 *  manager->activate_scene("loading"); // Will cause loading to happen again
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

class SceneLoadException : public std::runtime_error {};

class SceneBase:
    public Nameable,
    public Updateable {
public:
    typedef std::shared_ptr<SceneBase> ptr;

    SceneBase(WindowBase& window);
    virtual ~SceneBase();

    void _call_load();
    void _call_unload();

    void _call_activate();
    void _call_deactivate();

    bool is_loaded() const { return is_loaded_; }

protected:
    Property<SceneBase, WindowBase> window = { this, &SceneBase::window_ };

    virtual void load() = 0;
    virtual void unload() {}
    virtual void activate() {}
    virtual void deactivate() {}

    PipelineID prepare_basic_scene(
        StageID& new_stage,
        CameraID& new_camera,
        AvailablePartitioner partitioner=PARTITIONER_HASH
    );

    WindowBase* window_;

private:
    virtual void pre_load() {}
    virtual void post_unload() {}

    bool is_loaded_ = false;
};

template<typename T>
class Scene : public SceneBase, public Managed<T> {
public:
    Scene(WindowBase& window):
        SceneBase(window) {}

    void cleanup() override {
        _call_unload();
    }
};

}


#endif // SCENE_H
