
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
        register_scene<MyScene>("main");
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




