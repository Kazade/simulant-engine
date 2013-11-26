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

    bool operator<(const Colour& rhs) const {
        if(r < rhs.r) {
            return true;
        } else if (r == rhs.r) {
            if(g < rhs.g) {
                return true;
            } else if(g == rhs.g) {
                if(b < rhs.b) {
                    return true;
                }  else if(b == rhs.b) {
                    if(a < rhs.a) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    static const Colour WHITE;
    static const Colour BLACK;
    static const Colour GREY;
    static const Colour RED;
    static const Colour GREEN;
    static const Colour BLUE;
    static const Colour YELLOW;
    static const Colour PURPLE;
    static const Colour ORANGE;
};

}

#endif // COLOUR_H
