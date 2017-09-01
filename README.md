# Simulant

[![Join the chat at https://gitter.im/simulant-engine/Lobby](https://badges.gitter.im/simulant-engine/Lobby.svg)](https://gitter.im/simulant-engine/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

[![Build Status](https://travis-ci.org/Kazade/simulant-engine.svg?branch=master)](https://travis-ci.org/Kazade/simulant-engine)

## What is it?

Simulant is my accidental hobby engine. It started while I was reverse-engineering a model format and wanted to quickly and easily manipulate meshes. Over the next couple of years I chipped away at it here and there, using it for various little toy projects, but it grew out of control without any focus. Over time it became a dumping ground for useful graphics-related code, and depended on more and more third party libraries. 

In 2014, I decided that it there were a lot of nice things in Simulant, but it was a mess, and so I developed my first game "Rocks & Spaceworms" (https://play.google.com/store/apps/details?id=uk.co.kazade.rocks) with it and ported the code to Android. This allowed me to start focusing on removing dependencies and structuring the codebase like a real game engine. 

Since then the focus has been to remove dependencies, build an easy-to-use API, and performance. The engine now is already very different to the one that powered Rocks & Spaceworms. My future plan is to port the engine to "retro" consoles like the Dreamcast, and also Windows, OSX, iOS, and perhaps even PSP or NDS. Although you could use this engine for modern graphics (the material system is very powerful), it's generally aimed at providing mid-2000s style game graphics. 

There are some glaring holes in functionality (LOD, billboards, shadows, projected textures) but you can already build some impressive things, and those features will arrive eventually, hopefully sooner rather than later!

## Features

 * Easy to use API, and getting simpler all the time!
 * Flexible rendering pipeline
 * Complex material scripting format
 * Loading of .obj models and the X-Wing vs Tie Fighter .opt format
 * Loading of MD2 animated models
 * Loading of Q2 BSP files (needs work)
 * Loading of heightmap terrains from image files
 * Advanced spatial hash partitioning and culling
 * Loading of JPG, PNG, TGA, WAL, PCX images and more!
 * Shortcut functions for loading 2D sprites, 2D backgrounds and 3D skyboxes
 * Simple scene graph functions
 * UI widgets (Button, Label, Progress Bar)
 * Basic rigid body physics using the Bounce library
 * Procedural functions for generating spheres, cubes, capsules, circles and rectangles
 * Functions for creating lights, multiple viewports and cameras

## Screenshots

![screenshot 1](/screenshots/screenshot1.png?raw=true)
![screenshot 2](/screenshots/screenshot2.png?raw=true)
![screenshot 3](/screenshots/screenshot3.png?raw=true)
![screenshot 4](/screenshots/screenshot4.png?raw=true)

## Roadmap / TODO

 * Make geoms work using the new Octree in generic/tri_octree.h
 * Finish the GL 1.x renderer
 * Switch the GL 2.x renderer with a GL 4.x one
 * Implement render-to-texture
 * Fix lightmaps in the Q2 bsp loader
 * Implement multiple LOD for meshes
 * Build in support for shadows
 * Finish skybox management
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

To build:

```
    git submodule update --init 
    mkdir build && cd build
    cmake ..
    make
```

## How do I use it?

Well, it's pretty simple really...

```
#include <simulant/simulant.h>

class MyApp : public smlt::Application {
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
#include <simulant/simulant.h>

class MyApp : public smlt::Application {
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


