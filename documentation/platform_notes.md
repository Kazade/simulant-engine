# Platforms

Simulant is currently supported in some form on the following platforms:

 - Fedora (and probably other Linux distributions)
 - OSX
 - Dreamcast (some features unsupported)
 - Android (untested, probably needs work)

The following platforms will be targetted when possible (or when someone submits patches):

 - Windows
 - iOS
 - Web (via WASM)

And these platforms would be great!

 - Nintendo DS
 - PSP

## Dreamcast

Dreamcast support is currently a work in progress. The following things aren't supported yet:

 - Controller hotplugging
 - Mouse input
 - Lights
 - Compressed textures
 - Sound
 - Any kind of VMU support

There are also the following problems:

 - Low available memory: This is largely due to libGL allocating a large chunk of memory for clipping
   which can be fixed by increasing/decreasing the buffer based on average usage over time.
 - Low framerate: I'm not yet sure why, but the light_sample (spinning cube) runs at 36FPS, it should be
   10x that!
 - Unsupported operations: Point particles and text don't render, and doing so crashes the program
 
