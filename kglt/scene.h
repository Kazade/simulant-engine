#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <stdexcept>
#include <map>

#include "types.h"
#include "resource_manager.h"
#include "generic/auto_weakptr.h"
#include "generic/generic_tree.h"
#include "interfaces.h"
#include "generic/manager.h"

namespace kglt {

class WindowBase;
class UI;

class Scene;
class SceneImpl;
class Stage;
class GeomFactory;
class Background;

class Scene :
    public virtual ResourceManager,
    public virtual Updateable {

public:
    virtual ~Scene() {}

    virtual TextureID default_texture_id() const = 0;
    virtual MaterialID default_material_id() const = 0;

    virtual unicode default_material_filename() const = 0;

    virtual void enable_physics(std::shared_ptr<PhysicsEngine> engine) = 0;
    virtual PhysicsEnginePtr physics() = 0;
    virtual const bool has_physics_engine() const = 0;

    virtual MaterialID clone_default_material() = 0;
};

class SceneImpl:
    public Scene,
    public ResourceManagerImpl,
    public Loadable,
    public Managed<SceneImpl> {

public:
    SceneImpl(WindowBase* window);
    ~SceneImpl();

    void enable_physics(std::shared_ptr<PhysicsEngine> engine);
    PhysicsEnginePtr physics();
    const bool has_physics_engine() const;


    //UI Stages
    bool init();
    void update(double dt);

    MaterialID default_material_id() const;
    TextureID default_texture_id() const;

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(nullptr);
        obj._initialize();
    }

    unicode default_material_filename() const;

    MaterialID clone_default_material() {
        return new_material_from_file(default_material_filename());
    }

private:    
    TexturePtr default_texture_;
    MaterialPtr default_material_;

    void initialize_defaults();

    std::shared_ptr<PhysicsEngine> physics_engine_;

    friend class WindowBase;
};

}

#endif // SCENE_H_INCLUDED
