#ifndef COLOUR_H
#define COLOUR_H

namespace kglt {

struct Colour {
    float r, g, b, a;

    Colour():
        r(1.0), g(1.0), b(1.0), a(1.0) {}

    Colour(float r, float g, float b, float a):
        r(r), g(g), b(b), a(a) {}

    bool operator==(const Colour& rhs) const {
        return this->r == rhs.r && this->g == rhs.g && this->b == rhs.b && this->a == rhs.a;
    }

    static const Colour white;
    static const Colour black;
    static const Colour grey;
    static const Colour red;
    static const Colour green;
    static const Colour blue;
    static const Colour yellow;
    static const Colour purple;
};

}

#endif // COLOUR_H
