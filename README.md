# Simulant

**Development of Simulant has moved to [Gitlab](https://gitlab.com/simulant/simulant/)**

## What is it?

Simulant is a cross-platform general-purpose game engine written in C++.

It is designed with portability in mind, using minimal dependencies and targetting C++11 (rather than more modern versions of the language).

The aim of Simulant is to create an easy-to-use game engine to allow building homebrew for older games consoles, as well as creating a great developer-experience on modern platforms.

Simulant currently supports the following platforms:

 - Windows
 - Linux
 - SEGA Dreamcast
 - OSX (untested)
 - Sony PSP (incomplete)
 - Android (broken)
 
# Patreons

Simulant is supported by the following Patreons:

 - Dave Reichelt
 - clmjfr
 - Leonidas Antoniadis
 - Derek Pascarella

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

