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
 * Loading of .obj models and the X-Wing vs Tie Fighter .opt format
 * Loading of Q2 BSP files (needs work)
 * Loading of heightmap terrains from image files
 * Octree partitioning/culling (WIP)
 * Loading of JPG, PNG, TGA, WAL images
 * Shortcut functions for loading 2D sprites, 2D backgrounds and 3D skyboxes
 * Simple scene graph functions
 * UI layer based on the Nuklear library
 * Basic rigid body physics using the Qu3e library
 * Procedural functions for generating spheres, cubes, capsules, circles and rectangles
 * Procedural functions for generate a starfield texture (needs work)
 * Functions for creating lights, multiple viewports and cameras

## Roadmap / TODO

 * Fix up UI rendering after switch from libRocket -> Nuklear
 * Fix the onscreen joypad controller (broken with the switch to Nuklear)
 * Make geoms work using the new Octree in generic/tri_octree.h
 * Finish the GL 1.x renderer
 * Switch the GL 2.x renderer with a GL 4.x one
 * Implement render-to-texture
 * Fix lightmaps in the Q2 bsp loader
 * Implement animated meshes, starting with loading MD2
 * Implement multiple LOD for meshes
 * Introduce the concept of "bound" IDs, allowing `auto texture = texture_id.fetch();` to work if the ID was returned from new_X()
 * Make non-reference counted objects return raw pointers with fetched
 * Build in support for shadows
 * Fix the lighting sample
 * Finish skybox management
 * Add SDL1 support (for Dreamcast)
 * Add Dreamcast support (depends on GL 1.x)
 * Restore Android support
 * Port to OSX and Windows, then iOS
 * Implement VBO management in the GL 2.x renderer for improved performance 

## Building

Compiling the code requires CMake. Currently there are the following external dependencies:

 - ZLIB
 - SDL2
 - OpenGL
 - TinyXML
 - OpenAL
 - Kazbase (found on my GitHub)

To build:

```
    git submodule update --init 
    mkdir build && cd build
    cmake ..
    make
```

I'm working on bundling all dependencies except SDL2, OpenGL, OpenAL and ZLIB. kazbase is a bit of a dumping ground for useful code which I'm splitting
into separate libraries. Currently kglt depends on logging and unicode handling from there mainly.

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
        StageID stage_id;
        CameraID camera_id;
        prepare_basic_scene(stage_id, camera_id); // Set up a basic rendering pipeline

        auto stage = window->stage(stage_id); // Grab a handle to the stage
        MeshID mesh_id = stage->assets->new_mesh_as_rectangle(10.0, 5.0); // Load a mesh into the stage's asset manager
        ActorID actor_id = stage->new_actor_with_mesh(mesh_id); // Create an actor with the loaded mesh
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

You can build up sections of your scene by creating _Actors_ inside _Stages_, you can then combine your stages by building up a set of rendering
pipelines. For example, you can render Stage 1, to Viewport 1, with Camera 1 and target Texture 1. By building up these pipelines dynamically you
can build complex scenes.


# Documentation

## The Window, Stages and the RenderSequence

The _Window_ owns everything (pretty much). You can do the following with a Window:

* Create and delete Stages
* Create and delete resources (e.g. Meshes, Textures, Shaders and Materials) through the window->shared_assets property.
* Manipulate the render sequence

Stages are where you create your world by spawning _Actors_, you can think of them as a chunk of the overall scene. 

The RenderSequence is where you assemble your Stages for rendering. You add _Pipelines_ to the RenderSequence, and each Pipeline has the following:

* The Stage that this pipeline renders
* The CameraID of the camera to use
* The ViewportID of the viewport to render to
* An optional TextureID if you want to render to a texture rather than the framebuffer (not yet implemented)

For example, if you want to render a 2D overlay on your world you'll probably want to do that using a camera that has an orthographic projection. In that case, you'd do the following:

* Create a new stage ( window->new_stage() )
* Create a camera (window->new_camera())
* Build your 2D overlay in the stage (e.g. using new_mesh(), new_material(), new_actor() etc.)
* Manipulate the camera (e.g. window.camera(camera_id)->set_orthographic_projection())
* Finally add a stage to the pipeline (e.g. window->render(stage_id, camera_id()); )


