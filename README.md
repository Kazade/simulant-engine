# Kazade's GL Toolkit

## What is it?

While working on reverse engineering some obscure 3D model format, I needed a 
quick way to test file parsing, and procedually generating and rendering a mesh
from the model file. This library was spawned from that.

## How do I use it?

Well, it's pretty simple really...

```
#include "GL/window.h"

int main(int argc, char* argv[]) {

    kglt::Window window;
    
    while(window.update()) {}    

    return 0;
}
```

Crazy eh? Those two lines of code construct an OpenGL window, with an empty
scene and the program runs until the window is closed. 

But you wanted something more interesting than that right? OK, let's draw a
wireframe triangle:

    #include "GL/window.h"

    int main(int argc, char* argv[]) {

        kglt::Window window;

        //Set some rendering options
        window.scene().render_options.wireframe_enabled = true;

        //Create a new mesh
        kglt::Mesh& mesh = kglt::return_new_mesh(window.scene());
    
        //Add a submesh, add 3 vertices and a triangle made up of them
        mesh.add_submesh();
        mesh.submesh().add_vertex(-1.0, 0.0, 0.0);
        mesh.submesh().add_vertex(0.0, 1.0, 0.0);
        mesh.submesh().add_vertex(1.0, 0.0, 0.0);
        mesh.submesh().add_triangle(0, 1, 2);
    
        while(window.update()) {}    

        return 0;
    }


We can also draw a rectangle even more easily:


    #include "GL/window.h"

    int main(int argc, char* argv[]) {

        kglt::Window window;

        //Set some rendering options
        window.scene().render_options.wireframe_enabled = true;

        //Create a new mesh
        kglt::Mesh& mesh = kglt::return_new_mesh(window.scene());
    
        //Construct a rectangle 1.0 unit across, and half a unit high
        kglt::procedural::mesh::rectangle(mesh, 1.0, 0.5);
	
        //Move the rectangle out a little
        mesh.move_to(0.0, 0.0, -1.0);
	
        //Create an orthographic projection, 2 units high, with a 16:9 ratio (default)
        window.scene().viewport().set_orthographic_projection_from_height(2.0);
	
        while(window.update()) {}    

        return 0;
    }


# Documentation

## Objects

Everything visible in the Scene is a subclass of Object. An Object can have children, and they can have children etc. 

Each object has a position and an orientation relative to its parent. If this changes (or the Object's parent changes) an absolute position and orientation is recalculated. Pretty standard scene graph stuff.

## The Scene

The Scene owns everything. Like everything else it's a Object. It forms the root node of the scene you are trying to display. The Scene is also a factory for creating new objects. If you want to spawn a Mesh object for example, you would do so using window().scene().new_mesh();

Things you can spawn from the scene are:

 * Meshes
 * Textures
 * Shader programs
 * 2D Sprites
 * Fonts
 * Text objects

The Scene also contains other objects that aren't attached to it's heirarchy, these include:
 
 * The UI instance
 * Overlays
 * The Background instance

These objects are special, and so aren't traditional child nodes of the scene.

## The UI instance

The Scene object contains an instance of the UI class. This class is basically a layer above a 2D overlay that allows you to create common UI widgets such as labels. To create a label for example you would do something like this:

ui::LabelID lid = scene().ui().new_label();
ui::Label& label = scene().ui().label(lid);
label.set_text("This is a label");
label.set_position(0.1, 0.1); //UI positions are between 0.0 and 1.0 in both directions
label.set_foreground_colour(kglt::Colour(1.0, 1.0, 0.0, 1.0));
label.set_background_colour(kglt::Colour(0.0, 0.0, 0.0, 0.0));

Internally, new_label() creates a series of Mesh and Text objects and attaches them to an overlay.

## Overlays

Overlays are used to render 2D elements on top of the scene.

An Overlay is a different kind of root node than the Scene object. A Scene is rendered through a camera, which maintains a projection and a frustum. You can move the camera around a Scene.

Overlays are different, Overlays create a 2D orthographic projection that you can control. Object's can be attached to the Overlay and they will be rendered on top of the regular Scene. Cameras have no effect on an Overlay. Although an Overlay is an Object, it is not part of the Scene heirarchy and instead is the root of its own heirarchy. To render a 2D mesh on top of the scene, you would do the following:

Overlay& overlay = return_new_overlay(scene); //Create an overlay
Mesh& mesh = return_new_mesh(scene); //Create a mesh
mesh.set_parent(overlay); //Detach the mesh from the scene and attach it to the overlay

## Backgrounds

Each Scene has a background() property. Backgrounds are made up of one or more image layers that can be independently scrolled. Each layer is always positioned central to the viewport, and is rendered at the resolution of the image. You can clip the viewport to a certain section of a background by using set_visible_dimensions(X, Y);

For example, if you load a background layer from an image with a resolution of 512x256. You could then set the visible dimensions to 320x224 and use background().layer(0).scroll_x(amount) to move the viewport around.

Current constraints:

 * All layers must be the same dimensions
 * You cannot set the visible dimensions to greater than the layer size






