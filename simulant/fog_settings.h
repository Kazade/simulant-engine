#ifndef FOG_SETTINGS_H
#define FOG_SETTINGS_H

#include "colour.h"

namespace smlt {

enum FogType {
    FOG_TYPE_LINEAR,
    FOG_TYPE_EXP,
    FOG_TYPE_EXP2
};

class FogSettings {
public:
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    void set_linear(float start, float end);
    void set_exp(float density);
    void set_exp2(float density);
    bool is_enabled() const;
    FogType type() const;
    float exp_density() const;
    float linear_start() const;
    float linear_end() const;

    void set_colour(const Colour& colour);
    const Colour& colour() { return colour_; }
private:
    bool enabled_ = false;
    FogType type_ = FOG_TYPE_LINEAR;

    float density_ = 1.0f;
    float start_ = 0.0f;
    float end_ = 1.0f;

    Colour colour_;
};

}
#endif // FOG_SETTINGS_H
