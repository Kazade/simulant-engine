#include <random>

#include "random.h"

namespace random_gen {

static std::mt19937& engine() {
    static std::mt19937 eng;
    return eng;
}

void seed(uint64_t sd) {
    if(sd) {
        engine().seed(sd);
    } else {
        engine().seed(std::mt19937::default_seed);
    }
}

float random_float(float min, float max) {
    std::uniform_real_distribution<float> unif(min, max);
    return unif(engine());
}

int32_t random_int(int32_t min, int32_t max) {
    std::uniform_int_distribution<int32_t> unif(min, max);
    return unif(engine());
}

}
