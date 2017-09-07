
# Scene

A scene is designed to be a single screen in your game. For example, you might have a loading Scene, a menu Scene, etc.

## Creating a Scene

Scenes are subclasses of a class template called `Scene<T>`. To create your Scene class you must do the following:

 - Subclass `Scene<T>` but pass your class up as `T` (Search for the 'Curiously Recurring Template Pattern' for more info)
 - Add a pass-through constructor which takes a `Window*` and passes it to the parent constructor
 - Override the `void load()` method

Here's an example:

```

class MyScene : public Scene<MyScene> {
public:
    MyScene(Window* window):
        Scene<MyScene>(window) {}

    void load();
};

```

You can then register the Scene in your application's `init()` method:


```
    bool init() {
        scenes->register_scene<MyScene>("main");
        return true;
    }
```

Whichever Scene is registered with the name "main" is the first Scene instantiated when the application begins.

## Scene Methods

You can override the following methods in your Scene class:

 - load()
 - unload()
 - activate()
 - deactivate()
 - fixed_update()
 - update()
 - late_update()

`activate()` and `deactivate()` are called when you change to a different scene.

# Scene Management

`Scenes` are managed by the `SceneManager`, which is a property of `Application`. The application `SceneManager` is accessible directly from within a scene using the `scenes` property.

You can make a `Scene` current by calling `SceneManager::activate(name, behaviour)`. This will do a number of things:

 - If a `Scene` is already active, it will be deactivated.
 - If the behaviour is set to `SCENE_CHANGE_BEHAVIOUR_UNLOAD` then the old `Scene` will have its `unload()` method called. 
 - If `Scene::destroy_on_unload()` is true, then the old `Scene` will be deleted.
 - If this is the first time that this scene name has been activated, a new `Scene` will be instantiated.
 - If the new `Scene` hasn't been loaded, its `load()` method will be called.
 - `Scene::activate()` will be called on the new scene

You have a lot of control over this process:

 - You can load a `Scene` ahead of time, before activating it. 
 - You can prevent a `Scene` from being unloaded when deactivated.
 - You can prevent a `Scene` from being deleted when unloaded.
 
This means that you can (for example) create render pipelines that remain active even
once the `Scene` has been deactivated, and use that to implement transitions etc.



