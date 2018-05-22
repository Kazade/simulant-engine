#include "fog_settings.h"

namespace smlt {

void FogSettings::set_linear(float start, float end) {
    type_ = FOG_TYPE_LINEAR;
    start_ = start;
    end_ = end;
}

void FogSettings::set_exp(float density) {
    type_ = FOG_TYPE_EXP;
    density_ = density;
}

void FogSettings::set_exp2(float density) {
    type_ = FOG_TYPE_EXP2;
    density_ = density;
}

bool FogSettings::is_enabled() const {
    return enabled_;
}

FogType FogSettings::type() const {
    return type_;
}

void FogSettings::set_colour(const Colour &colour) {
    colour_ = colour;
}

float FogSettings::exp_density() const {
    return density_;
}

float FogSettings::linear_start() const {
    return start_;
}

float FogSettings::linear_end() const {
    return end_;
}

}
