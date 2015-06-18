# Kazade's GL Toolkit

[![Join the chat at https://gitter.im/Kazade/KGLT](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Kazade/KGLT?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## What is it?

While working on reverse engineering some obscure 3D model format, I needed a
quick way to test file parsing, and procedually generating and rendering a mesh
from the model file. This library was spawned from that, and before long it
became a full blown game engine!

## Features

 * Easy to use API, and getting simpler all the time!
 * Flexible rendering pipeline
 * Complex material scripting format
 * Built-in Lua console (hit '~' , or '`' on a UK keyboard... top left!)
 * Manipulate the scene in realtime using Lua
 * Loading of .obj models and the X-Wing vs Tie Fighter .opt format
 * Loading of Q2 BSP files (needs work)
 * Octree partitioning/culling (WIP)
 * Loading of JPG, PNG, TGA images
 * Shortcut functions for loading 2D sprites, 2D backgrounds and 3D skyboxes
 * Simple scene graph functions
 * HTML/CSS based UI renderer based on libRocket
 * Procedural functions for generating spheres, cubes, capsules, circles and rectangles
 * Procedural functions for generate a starfield texture (needs work)
 * Functions for creating lights, multiple viewports and cameras

## How do I use it?

Well, it's pretty simple really...

```
#include <kglt/kglt.h>

class MyApp : public kglt::Application {
public:
    bool do_init() {
        return true;
    }

    void do_step(double dt) {

    }

    void do_cleanup() {

    }
};


int main(int argc, char* argv[]) {
    MyApp app;
    return app.run();
}
```

Crazy eh? Those few lines of code construct an OpenGL window, with an empty
scene and the program runs until the window is closed.

But you wanted something more interesting than that right? OK, let's draw a
rectangle:

```
#include <kglt/kglt.h>

class MyApp : public kglt::Application {
public:
    bool do_init() {
        MeshID mesh_id = window().stage()->new_mesh_as_rectangle(10.0, 5.0);
        ActorID actor_id = window().stage().new_actor_with_mesh(mesh_id);
        return true;
    }

    void do_step(double dt) {

    }

    void do_cleanup() {

    }
};


int main(int argc, char* argv[]) {
    MyApp app;
    return app.run();
}

You can build up sections of your scene by creating _Actors_ inside _Stages_, you can then combine your stages by building up a set of rendering
pipelines. For example, you can render Stage 1, to Viewport 1, with Camera 1 and target Texture 1. By building up these pipelines dynamically you
can build complex scenes.
```

# Documentation

## The Window, Stages and the RenderSequence

The _Window_ owns everything (pretty much). You can do the following with a Window:

* Create and delete Stages
* Create and delete resources (e.g. Meshes, Textures, Shaders and Materials)
* Manipulate the render sequence

When you create a Window, a single Stage is created; this is the default Stage. Stages are where you create your world by spawning _Actors_, you can think of them as a chunk of the overall scene. Generally speaking most objects that you create should belong to the default stage. There's nothing particularly special about the default Stage aside from it being created automatically and it being automatically connected to a pipeline in the render sequence for you.

The RenderSequence is where you assemble your Stages for rendering. You add _Pipelines_ to the RenderSequence, and each Pipeline has the following:

* The Stage that this pipeline renders
* The CameraID of the camera to use
* The ViewportID of the viewport to render to
* An optional TextureID if you want to render to a texture rather than the framebuffer (not yet implemented)

For example, if you want to render a 2D overlay on your world you'll probably want to do that using a camera that has an orthographic projection. In that case, you'd do the following:

* Create a new stage ( window->scene().new_stage() )
* Build your 2D overlay in the subscene (e.g. using new_mesh(), new_material(), new_actor() etc.)
* Manipulate the stage's camera (e.g. window.camera().set_orthographic_projection())
* Finally add a stage to the pipeline (e.g. window().render_sequence().add_pipeline(stage.id(), camera().id(), ViewportID(), TextureID(), 100) );

The 100 in the above example is the priority of this stage, the stage that renders the default subscene has a priority of 0, so giving your overlay subscene a priority of 100 would mean it would be renderered after the default.
