# Kazade's GL Toolkit

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
#include "GL/window.h"

int main(int argc, char* argv[]) {

    kglt::Window::ptr window = kglt::Window::create();
    while(window->update()) {}

    return 0;
}
```

Crazy eh? Those two lines of code construct an OpenGL window, with an empty
scene and the program runs until the window is closed. 

But you wanted something more interesting than that right? OK, let's draw a
rectangle:

```
#include "kglt/kglt.h"

int main(int argc, char* argv[]) {

    kglt::Window::ptr window = kglt::Window::create();
    kglt::SubScene& scene = window->scene().subscene();

    //Create a new mesh
    kglt::Mesh& mesh = scene.mesh(scene.new_mesh());

    //Construct a rectangle 1.0 unit across, and half a unit high
    kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 0.5);

    kglt::Entity& entity = scene.entity(scene.new_entity(mesh.id()));
    
    //Move the rectangle out a little
    entity.move_to(0.0, 0.0, -1.0);

    //Create an orthographic projection, 2 units high, with a 16:9 ratio (default)
    window->viewport().set_orthographic_projection_from_height(2.0);

    while(window->update()) {}    

    return 0;
}
```

# Documentation

## Scenes, SubScenes and the Pipeline

The Scene owns everything (pretty much). Every Window has a single Scene object. You can do the following with a Scene:

* Create and delete SubScenes
* Create and delete resources (e.g. Meshes, Textures, Shaders and Materials)
* Manipulate the rendering pipeline

When you create a Window, the Scene is created along with a single SubScene; this is the default SubScene. SubScenes are where you create your world by spawning "Entity"s, you can think of them as a chunk of the overall Scene. Generally speaking most objects that you create should belong to the default subscene. There's nothing particularly special about the default SubScene aside from it being created automatically and it being automatically connected to a stage in the rendering pipeline for you.

The Pipeline is where you assemble your SubScenes for rendering. You add stages to the Pipeline, and each stage has the following:

* The SubScene that this stage renders
* The CameraID of the camera to use
* The ViewportID of the viewport to render to
* An optional TextureID if you want to render to a texture rather than the framebuffer (not yet implemented)

For example, if you want to render a 2D overlay on your world you'll probably want to do that using a camera that has an orthographic projection. In that case, you'd do the following:

* Create a new subscene ( window->scene().new_subscene() )
* Build your 2D overlay in the subscene (e.g. using new_mesh(), new_material(), new_entity() etc.)
* Manipulate the subscene's camera (e.g. subscene.camera().set_orthographic_projection())
* Finally add a stage to the pipeline (e.g. subscene.scene().pipeline().add_stage(subscene.id(), subscene.camera().id(), ViewportID(), TextureID(), 100) );

The 100 in the above example is the priority of this stage, the stage that renders the default subscene has a priority of 0, so giving your overlay subscene a priority of 100 would mean it would be renderered after the default.

## The "extra" namespace

KGLT is designed so that the core is as streamlined and minimalist as possible. The idea being that everything can be built using a combination of the core objects (e.g. Meshes, and Materials). Still, this means that creating objects such as animated sprites is quite a manual process. The "extra" namespace contains classes that manage the core objects for you to provide easy common functionality. For example, the kglt::extra::Sprite class allows you to construct characters with several animations easily:
    
    auto sprite = kglt::extra::Sprite::create(scene); //Create a sprite
    sprite->add_animation("anim_1", frames); //Frames is an array of texture IDs
    sprite->set_current_animation("anim_1");
    sprite->move_to(0, 0, -1);

These additional classes aren't owned by the Scene, but they do automatically take care of core objects for you.

