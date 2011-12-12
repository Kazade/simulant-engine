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

    GL::Window window;
    
    while(window.update()) {}    

    return 0;
}
```

Crazy eh? Those two lines of code construct an OpenGL window, with an empty
scene and the program runs until the window is closed. 

But you wanted something more interesting than that right? OK, let's draw a
wireframe triangle:


```
#include "GL/window.h"

int main(int argc, char* argv[]) {

    GL::Window window;

    //Set some rendering options
    window.scene().render_options.backface_culling_enabled = false;
    window.scene().render_options.wireframe_enabled = true;
    window.scene().render_options.textures_enabled = false;

    //Create a mesh
    GL::MeshID m = window.scene().new_mesh();
    
    //Get a handle to the mesh
    Mesh& mesh = window.scene().mesh(m);
    
    //Add a submesh, add 3 vertices and a triangle made up of them
    mesh.add_submesh();
    mesh.submesh().add_vertex(-1.0, 0.0, 0.0);
    mesh.submesh().add_vertex(0.0, 1.0, 0.0);
    mesh.submesh().add_vertex(1.0, 0.0, 0.0);
    mesh.submesh().add_triangle(0, 1, 2);
    
    while(window.update()) {}    

    return 0;
}
```

