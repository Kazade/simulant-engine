#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>

namespace random_gen {

void seed(uint64_t sd=0);
float random_float(float min, float max);
int32_t random_int(int32_t min, int32_t max);

}

#endif // RANDOM_H
