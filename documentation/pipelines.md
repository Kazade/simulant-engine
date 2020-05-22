# Render Pipelines

A render pipeline (just called a `Pipeline` in the API) is a combination of a `Stage`, `Camera` and some kind of render target (often the framebuffer) which controls the visual output of your scene. `Pipelines` can be combined to "layer up" the finally displayed frame.

`Pipelines` are controlled by the `Compositor` which is a property of the `Window` and each frame are run in order to generate the final image. For example, if you want to create a 3D scene, with a 2D UI layer you may do the following:

 1. Create a `Stage` and a perspective `Camera` for the 3D portion
 2. Create a `Stage` and an orthographic `Camera` for the UI
 3. Create a `Pipeline` to render the 3D scene using the perspective camera
 4. Create a `Pipeline` to render the UI using the orthographic camera, but with a higher `RenderPriority` than the 3D scene

This is how it might look in code:

```
auto stage = window->new_stage();
auto persp_camera = stage->new_camera();

auto overlay = window->new_stage();
auto ortho_camera = overlay->new_camera();

auto pipeline1 = compositor->render(stage, persp_camera);
auto pipeline2 = compositor->render(
    overlay, ortho_camera
)->set_priority(RENDER_PRIORITY_FOREGROUND);

pipeline1->activate();
pipeline2->activate();
```

Now `StageNodes` added to `stage` will be rendered in 3D before UI elements added to `overlay`.

# Linking Pipelines to Stages

Often you will build up your scenes in the `load()` method of your `Scene` subclass. It's probable that this scene building
will happen in a background thread. Activating pipelines in `load()` in this scenario is dangerous as any stage that's attached
to an activated `Pipeline` will be updated in the main thread which will cause deadlocks and crashes if you're also manipulating
it concurrently. It's advised that you activate your pipelines in your `Scene's` `activate()` method, and that you deactivate
them in your `Scene's` `deactivate()` method.

This boilerplate overriding of `Scene::activate()` and `Scene::deactivate()` can become repetitive, so Simulant provides a helper method
called `Scene::link_pipeline`. Once you link a `Pipeline` to a `Scene` it will activate and deactivate along with it.

```
auto pipeline1 = compositor->render(stage, persp_camera);
link_pipeline(pipeline1); // pipeline1 will activate when this Scene is activated
```

# Naming Pipelines

Pipelines are `Nameable` meaning that you can set a name and then find them later by that name:

```
compositor->render(stage, camera)->set_name("pipeline1");

auto p1 = window->find_pipeline("pipeline1");
```

If there is no pipeline with that name, then you will get a null PipelinePtr returned.
