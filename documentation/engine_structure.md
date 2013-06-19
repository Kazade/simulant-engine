# Engine Structure

The KGLT architecture can be divided into three parts:

 - Render system
 - Scene management
 - The "extra" namespace
 
## Render System

The render system in KGLT is in constant flux at the moment, but the general
architecture has been established. The rendering process is based around a series
of pipelines. Each pipeline take a StageID (or UIStageID), CameraID and ViewportID 
as inputs, and an optional target TextureID as an output. If the TextureID is omitted, 
then the output of the pipeline is applied directly to the framebuffer.

Pipelines also have an ordering value. The higher the ordering, the later in the
render sequence the pipeline runs. The RenderSequence class manages Pipelines
and is responsible for processing them in order.

The Render System is structured in this way for flexibility. Imageine for a second that
you are writing a game and you want to show the CCTV camera in the next room on
an in-game TV monitor. You could do this by creating a Camera representing the view
of the CCTV camera, and then add a Pipeline that uses this Camera's ID, but outputs to the
TextureID of the TV monitor. Then the final Pipeline would use the CameraID of the player
and output to the framebuffer.
