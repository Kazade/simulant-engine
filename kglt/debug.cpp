
#include "debug.h"

namespace kglt {

Debug::Debug(Scene &scene):
    scene_(scene) {

}

void Debug::draw_line(const Vec3 &start, const Vec3 &end, const Colour &colour, double duration, bool depth_test) {

}

void Debug::draw_ray(const Vec3 &start, const Vec3 &dir, const Colour &colour, double duration, bool depth_test) {
    draw_line(start, start+dir, colour, duration, depth_test);
}

void Debug::draw_point(const Vec3 &position, const Colour &colour, double duration, bool depth_test) {

}

}
