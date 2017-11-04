
# Cameras

Cameras are treated as physical objects within a Stage. They are StageNodes (like Actors) and can be moved, rotated and made children (or parents) of other StageNodes.

When you define a render pipeline (normally through `window.render(stage_id, camera_id)`) the Camera specified must exist within the Stage being rendered. 
