# Renderers

Simulant supports multiple renderers, and by default it ships with two:

 - OpenGL 4.5 (for most systems)
 - OpenGL 1.x (1.0 + Extensions supported by the SEGA Dreamcast SDK)

There are outstanding issues with the renderers, in particular:

 - The OpenGL 4.5 renderer has poor buffer management. Much of the VBO handling is handled by the Mesh + SubMesh, when it should be
   handled by the renderer. The current system has one VBO of vertex attribute data per-Mesh which is very inefficient. 
 - The OpenGL 1.x renderer is largely untested and the SEGA Dreamcast libGL library (GLdc) needs work.
 - The renderers are selected at compile time, when they should be switchable on program launch (e.g. `SIMULANT_RENDERER=GL1x ./myapp`).
 - Android should have an OpenGL ES renderer.
