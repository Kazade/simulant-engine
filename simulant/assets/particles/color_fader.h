#pragma once

#include "../particle_script.h"
#include "curves.h"

namespace smlt {

class ColorFader:
    public Manipulator {
public:
    ColorFader(ParticleScript* script, const std::vector<Color>& colors, bool interpolate):
        Manipulator(script, "color_fader"),
        colors_(colors),
        interpolate_(interpolate) {}

private:
    void do_manipulate(ParticleSystem*, Particle* particles, std::size_t particle_count, float) const {
        auto size = (float) colors_.size();
        float fsize = float(size);
        auto particle = particles;
        for(auto i = 0u; i < particle_count; ++i, ++particle) {
            const float e = (particle->lifetime - particle->ttl);
            const float n = smlt::fast_divide(e, particle->lifetime);
            const float fsizen = fsize * n;

            uint8_t color = smlt::clamp(fsizen, 0.0f, fsize);

            particle->color = colors_[color];

            if(interpolate_) {
                const float f = fsizen - std::floor(fsizen);
                auto next_color = colors_[std::min((uint32_t) color + 1, (uint32_t) size - 1)];
                particle->color = (particle->color * (1.0f - f)) + (next_color * f);
            }
        }
    }

    std::vector<Color> colors_;
    bool interpolate_ = true;
};

}
