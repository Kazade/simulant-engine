# Simulant

**Development of Simulant has moved to [Gitlab](https://gitlab.com/simulant/simulant/)**

## What is it?

Simulant is a cross-platform general-purpose game engine written in C++.

It is designed with portability in mind, using minimal dependencies and targetting C++11 (rather than more modern versions of the language).

The aim of Simulant is to create an easy-to-use game engine to allow building homebrew for older games consoles, as well as creating a great developer-experience on modern platforms.


## Status

Simulant is currently in the *Beta* stage. This means that you can use it to build games, but you may come across bugs or shortcomings. 

This is the perfect time to get involved in the development of Simulant! If you find bugs, please report them, and if you think of cool new features, then feel free to create an issue for those too!

## History

Simulant was my accidental hobby engine. It started while I was reverse-engineering a model format and wanted to quickly and easily manipulate meshes. Over the next couple of years I chipped away at it here and there, using it for various little toy projects, but it grew out of control without any focus. Over time it became a dumping ground for useful graphics-related code, and depended on more and more third party libraries. 

In 2014, I decided that it there were a lot of nice things in Simulant, but it was a mess, and so I developed my first game "Rocks & Spaceworms" (https://play.google.com/store/apps/details?id=uk.co.kazade.rocks) with it and ported the code to Android. This allowed me to start focusing on removing dependencies and structuring the codebase like a real game engine. 

Since then the focus has been to remove dependencies, build an easy-to-use API, and performance. The engine now is already very different to the one that powered Rocks & Spaceworms. My plan is to port the engine to "retro" consoles like the Dreamcast, and also Windows, OSX, iOS, and perhaps even PSP or NDS. Although you could use this engine for modern graphics (the material system is very powerful), it's generally aimed at providing mid-2000s style game graphics. 


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

## Building

Compiling the code requires CMake. Currently there are the following external dependencies:

 - ZLIB
 - SDL2
 - OpenGL
 - OpenAL

To build:

```
    git submodule update --init 
    mkdir build && cd build
    cmake ..
    make
```

## How do I use it?

Have a read of the [documentation](https://simulant.dev/docs) and follow the installation instructions for your OS.

