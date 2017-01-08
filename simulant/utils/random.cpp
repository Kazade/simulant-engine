//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

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
