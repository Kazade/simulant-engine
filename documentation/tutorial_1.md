# Writing your First Simulant Application

Welcome to the first Simulant tutorial!

In this tutorial we'll set out to do the following:

1. Create a new Simulant project
2. Learn about the Simulant project structure
3. Compile and run your Simulant project
4. Learn how render pipelines work
5. Load and manipulate a 3D model

This tutorial assumes that you are running the Fedora 29 operating system (either natively, or in a Virtual Machine) and you have followed the "Prerequisites" and "Installation" sections in the [Fedora installation instructions](install_fedora.md).

This tutorial also assumes the following fundamental knowledge:

 - You have some knowledge of C++ or similar (e.g. C)
 - You are comfortable using the terminal
 - You can find your way around a Linux OS
 
If you get stuck at any point, head over to the Simulant Discord server.

If you are using another Linux distribution the instructions should be very similar.

> **Why Fedora?**
> 
> Fedora is a user-friendly operating system which stays up-to-date with the latest advances
> in the Linux ecosystem. It has good support for Docker and cross-compilation which Simulant depends on. 

## Starting your project

We're going to create a project called "monsters". Assuming you have Simulant correctly installed then this is very simple. Open a terminal and run the following commands:

```
mkdir ~/Projects
cd ~/Projects
simulant start monsters
```

This will create a Projects folder in your home folder, navigate to it, and then start creating your Simulant project.

The `start` command will download several things:

 - The Simulant engine compiled for multiple platforms
 - The Simulant core assets
 - The Simulant test generator
 
It may take a little while. When the process finishes you'll find a new directory called "monsters" has been created.

Now open the file manager and navigate to the Projects folder.

![figure 1](/documentation/images/tutorial_1_1.png?raw=true)

As you can see there are a number of folders and files here, let's go through the folders first:

 - `assets` - This is where you put your game assets like textures, models, materials etc.
 - `libraries` - This is where the Simulant libraries live. If you depend on third party libraries you can also put them here.
 - `packages` - This folder is where your final distributable packages are generated.
 - `sources` - This is where the source code of your project are kept.
 - `tests` - This is where Simulant looks for unit tests for your project.
 - `tools` - This is where binary tools are found.

Now, there are a couple of files you should know about:

 - `simulant.json` - This is a configuration file for your project.
 - `CMakeLists.txt` - Simulant uses CMake to compile your project, this controls that.

It's recommended that you edit simulant.json at this point and customise it for your project.

## Building and Running your Project
 
Now, let's get something up and running! From a terminal, run the following commands:

```
cd ~/Projects/monsters
simulant build
```

This will compile your project executable. It will actually generate two binaries:

 - Your project executable which gets the same name as your project
 - A `tests` executable that runs your unit tests
 
There'll be more on testing later.

Once your project has built, you can run it with the following command:

```
simulant run
```

If all goes well you should see a Simulant splash screen, followed by a blank screen. Press escape to quit.

If you have Docker installed, you can also build your project for other platforms, for example:

```
simulant build dreamcast
simulant build windows
```

And, if you want to rebuild and then run your project in a single command:

```
simulant run --rebuild
```

## Scenes

A Simulant application is made up of a number of "Scenes". These are classes which subclass
the `Scene` class template. A `Scene` represents a single portion of your game, for example you might have a `Scene` for the game's menu, you might have another for the gameplay, another for the game over screen etc.

Scenes have a number of methods which you can override:

 - `on_load()` - This is where you build the scene. This might involve loading assets or creating
   actors, particle systems etc. More on that later.
 - `on_unload()` - The opposite of load, this is where you tear down everything you built up.
 - `on_activate()` - This is called when your `Scene` becomes the active scene.
 - `on_deactivate()` - This is called when your `Scene` stops being the active one.
 - `on_update(dt)` - This is called once per frame, and passes in a delta time value. This is where you'd move objects over time.
 - `on_fixed_update(step)` - This is like update, but rather than being called once per frame, it's called 60 times per second. This might mean it's called more than once in a frame, or maybe not at all.
 
You must at least override `on_load()`, the others are optional.

The `Scene` has a number of properties that you have access to:

 - `window` - This is the game window, you can access most of the Simulant API through this.
 - `app` - This is the application
 - `input` - The input manager
 - `scenes` - The scene manager. You can use this to activate another `Scene`
 - `compositor` - The compositor is used to build your visual output using layers.

Once you've created your `Scene` class, you need to register it in your `Application::init` method (in your main.cpp file).

```
scenes->register_scene<MySceneClass>(name, ...);
```

The first argument is a name for your `Scene`, then any additional arguments are passed to your
`Scene` constructor.

When registering, the name "main" is special. If you call a `Scene` "main" it will be the first `Scene` called when your game starts. By default the "main" `Scene` is the simulant splash screen and the default generated `Scene` is called "ingame".

# The building blocks of a Scene

A `Scene` in Simulant is a tree of StageNodes with the `Scene` class itself at the root of the tree.

You construct your scene by creating specialised `StageNode` subclasses which perform different tasks. There are a handful of built in `StageNode` subclasses, but to build out your game logic you will likely need to create many of your own.

Some of the built-in StageNodes include:

- `Stage` - this can be thought of as a "group" or "container" node. It effectively allows you to group other nodes together and apply transformations to them as a whole.
- `Actor` - an actor is an instance of a `Mesh` asset.
- `PrefabInstance` - a prefab instance is the root of an instantiated `Prefab` asset. It's effectively another "container" node, but also has animation properties to control animation of the descendent nodes.
- `ParticleSystem` - a particle system is a node that manages a collection of particles (created from a `ParticleScript` asset).
- `Camera` - camera is a special node that is passed to `Layers` created through the compositor.
- `Light` - a light is a special node that is used to illuminate the scene.

You create nodes in one of two ways:

 - By calling the `create_child<T>(...)` method on an existing node (e.g. the Scene itself)
 - By calling `create_node<T>(...)` on an existing node. This will create an "orphan" node which won't do anything unless attached to the scene tree.

The easiest way to create a scene graph is to load a GLTF file as a prefab, and then use that to instantiate the scene. You can then render that prefab by
creating a new layer in the compositor. The following code would live inside your `on_load()` override:

```
auto camera = create_child<Camera3D>();
auto prefab = assets->load_prefab("path/to/prefab.glb");
auto instance = create_child<PrefabInstance>(prefab);
auto layer = compositor->create_layer(instance, camera);
```

You can create multiple layers which are composited by priority (similar to the layers in a paint program):

```
auto camera = create_child<Camera3D>();
auto prefab = assets->load_prefab("path/to/prefab.glb");
auto instance = create_child<PrefabInstance>(prefab);
auto layer = compositor->create_layer(instance, camera);

auto ui_camera = create_child<Camera2D>();
auto ui_manager = create_child<UIManager>();
auto layer2 = compositor->create_layer(ui_manager, ui_camera);
layer2->set_render_priority(RENDER_PRIORITY_FOREGROUND);
```

Before building your scene it helps to plan your layers in advance, and then create `Stages` to separate them within your scene graph for individual rendering.
