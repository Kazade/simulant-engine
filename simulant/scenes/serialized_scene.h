#pragma once

#include "asset.h"
#include "scene.h"

namespace smlt {

template<typename T>
class SerializedScene:
    public Scene<T>,
    public Asset {

public:
    SerializedScene(Window* window, const std::string& file_path):
        Scene<T>(window),
        file_path_(file_path) {}

    /* Serialized scenes are loaded by the
     * SceneManager, override pre_load
     * or post_load if you need to do additional
     * work */
    bool load() override final {
        assert(file_path_);

        if(!pre_load()) {
            L_ERROR("Pre-load returned false, not loading scene");
            return false;
        }

        window->loader_for(file_path_)->into(*this);
        post_load();
        return true;
    }

private:
    friend class SceneManager;

    /* Called prior to loading the scene from
     * file. Return false to cancel loading */
    virtual bool pre_load() {}

    /* Called after loading the scene from file */
    virtual void post_load() {}

    std::string file_path_;
};


/* Scripted scenes are scenes that contain
 * all of their logic in script files, they
 * must be loaded from disk using
 * the SceneManager.
 */
class ScriptedScene:
    public SerializedScene<ScriptedScene> {

public:


};

}
