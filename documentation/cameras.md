
# Cameras

Camera''s belong to the window, this is because they are essential to the rendering process. This also means they are unrelated to stages.

It's a common use case to want to attach a camera to an entity, so a camera can have a CameraProxy in one of the stages which when manipulated
will update its associated camera. You can create a CameraProxy by calling stage.host_camera(CameraID).
