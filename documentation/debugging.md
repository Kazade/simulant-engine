# Debugging

## Debug Drawing

Sometimes while debugging your project it's useful to quickly visualize things like vectors and points on-screen to aid problem solving.

In Simulant, every `Stage` has a `debug` property. The `Debug` object allows quickly adding points and lines with the following methods:

 - `draw_line(const Vec3 &start, const Vec3 &end, const Colour &colour, double duration, bool depth_test)`
 - `draw_ray(const Vec3 &start, const Vec3 &dir, const Colour &colour, double duration, bool depth_test)`
 - `draw_point(const Vec3 &position, const Colour &colour, double duration, bool depth_test)`

Most of the arguments are self explanatory with the exception of `duration`. `duration` is the length of time to persist this line or point. By default this is 0 seconds (meaning you need to call `draw_X()` every frame), however if you want to persist a debug element across frames you can specify the length of time that the debug element should live.

### Sample Usage

```
    stage->debug->draw_line(Vec3(0, 0, 0), Vec3(1, 1, 1), Colour::RED);
    stage->debug->draw_ray(Vec3(0, 0, 0), Vec3(0, 1, 0));  // Defaults to WHITE

    // Draws a blue point for 1 second that won't be depth tested
    stage->debug->draw_point(Vec3(0, 0, 0), Colour::BLUE, 1.0f, false);
```
