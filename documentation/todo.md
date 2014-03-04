
# TODO

KGLT is a one-man project, and it is already way too large for that! Lots of areas of 
the engine are flakey, untested, or just downright badly thought out!

My aim is that, if I keep hacking as and when I can, eventually it will be a useful,
easy to use game engine. Until then, here are some of the things that need doing:

 - Make all object accessors return a std::shared_ptr typedef, or ProtectedPtr for consistency, no more references
 - Clean up the material cloning stuff which is ugly and error prone
 - Increase the use of shared abstract interfaces. Add them to interfaces.h
 - Speed up the rendering pipeline stuff, try to make it more understandable and the API nicer (e.g. rename/replace add_pipeline)
 - Animated meshes (keyframe and skeletal), probably means big changes to the Mesh structure
 - Extend the "Screen" concept that the loading screen uses, make it easy to background load multiple screens and switch between them
 - Shadows
 - Lighting (half-complete)
 - Reflections
 - Render to texture (e.g. the texture parameter to add_pipeline)
 - Improve LUA support (what is there now is ultra basic)
 - More Sprite classes/improvements
 - Replacing ODE with Bullet as the default physics backend
 - Making Octree culling actually work
 - Support for (at least) MD2, MD3, MD5 and MS3D models
 - Performance improvements
 - Unit tests... lots of unit tests
 - Performance improvements
 - Scriptable particle system
 - Improvements to the Material format/classes
 - Special effects (fog, projective textures etc.)
 - Terrain classes with Quadtree culling
 - Animated, dynamic sky spheres/weather systems
 - Culling improvments, testing of the dynamic octree partitioner
 - Funky anti-aliasing
 - Mouse input smoothing
 - Steam controller compatibility 
 - General game controller improvements using SDL 2 features
 - Better world geometry subdivision/culling
 - Better debugging tools
 - Memory pooling
 
