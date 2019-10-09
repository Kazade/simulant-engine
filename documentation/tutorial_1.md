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

 - `load()` - This is where you build the scene. This might involve loading assets or creating
   actors, particle systems etc. More on that later.
 - `unload()` - The opposite of load, this is where you tear down everything you built up.
 - `activate()` - This is called when your `Scene` becomes the active scene.
 - `deactivate()` - This is called when your `Scene` stops being the active one.
 - `update(dt)` - This is called once per frame, and passes in a delta time value. This is where you'd move objects over time.
 - `fixed_update(step)` - This is like update, but rather than being called once per frame, it's called 60 times per second. This might mean it's called more than once in a frame, or maybe not at all.
 
You must at least override `load()`, the others are optional.

The `Scene` has a number of properties that you have access to:

 - `window` - This is the game window, you can access most of the Simulant API through this.
 - `app` - This is the application
 - `input` - The input manager
 - `scenes` - The scene manager. You can use this to activate another `Scene`

Once you've created your `Scene` class, you need to register it in your `Application::init` method (in your main.cpp file).

```
scenes->register_scene<MySceneClass>(name, ...);
```

The first argument is a name for your `Scene`, then any additional arguments are passed to your
`Scene` constructor.

When registering, the name "main" is special. If you call a `Scene` "main" it will be the first `Scene` called when your game starts. By default the "main" `Scene` is the simulant splash screen and the default generated `Scene` is called "ingame".

## Render Pipelines

Now that you have your project created and running, let's talk about how Simulant renders
to the screen.

You control rendering in Simulant via `Pipelines`. A `Pipeline` combines the following:

 - The ID of `Stage` you want to render.
 - The ID of a `Camera` inside the `Stage` that you want to use.
 - Optionally a `Viewport`.
 - Optionally a `Texture` to render to (rather than the window)
 - A render priority

Creating a pipeline is easy:

```
auto stage = window->new_stage();  // Create a stage
auto camera = stage->new_camera();  // Create a camera within the stage
auto pipeline = window->render(stage, camera);  // Create your pipeline
```

It is recommended you activate and deactivate your pipeline in the `activate()` and `deactivate()` methods of your `Scene`.
Pipelines are created deactivated.

```
void MyScene::load() {
    // ...
    
    pipeline_ = window->render(stage, camera);
}

void MyScene::activate() {
    pipeline_->activate();
}

void MyScene::deactivate() {
    pipeline_->deactivate();
}

```

You can create multiple pipelines and this gives you the control to do some cool things:

 - Render the same stage to different viewports with different cameras (multiplayer)
 - Render a stage to a texture, then use that texture on a mesh
 - You can prolong the life of a pipeline across `Scenes` to create transitions

## Loading a 3D Model

Let's now talk about the `Stage`. You created a `Stage` earlier, and a `Camera` within it so that you could build a `Pipeline`. 

A `Stage` is the root of a heirarchical set of `StageNodes` which you can manipulate and render. `StageNodes` (e.g. `Actors`, `Cameras`, `Lights` etc.) can be moved around, rotated, and parented to other `StageNodes` to build up your scene.

The most common object you will create is an `Actor`. An `Actor` is normally (but not always) associated with some kind of `Mesh` for rendering. In your `load()` method, it's very straightforward to load a 3D mesh, and attach it to an `Actor`:

```
auto mesh_id = stage->assets->new_mesh_from_file("models/mymesh.obj");
auto actor = stage->new_actor_with_mesh(mesh_id);
actor->move_to(0, 0, -10);
```

That's it!

In Simulant the `Camera` looks down the negative Z axis by default. What we've done here is 
loaded a 3D model asset, created an `Actor` that uses it, then moved that `Actor` so that it can be seen by the `Camera`.

You'll notice that we accessed a property of the `Stage` called "assets". Every `Stage` has its own `AssetManager`, when the `Stage` is destroyed then those assets are destroyed too. Sometimes you want to share assets across `Stages` though, and you can do that with the `shared_assets` property of the `Window`.

One last thing to be aware of is that assets are ref-counted. Once you access an asset, or attach it to an `Actor` it will be destroyed when all users of that asset are destroyed.

You can turn this behaviour off by passing `smlt::GARBAGE_COLLECT_NEVER` as a second argument to `new_mesh_from_file`.

## Summary

That's quite a lot for an introduction! You've learned that Simulant is structured around `Scenes` at a high-level, the rendering system is controlled by `Pipelines`, and you build up your visible scene by manipulating `Cameras` and `Actors` within a `Stage`.

Now, go ahead an experiment!


