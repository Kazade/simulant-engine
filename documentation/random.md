# Random

Simulant ships with a basic class for generating pseudo-random numbers and vectors. 

## RandomGenerator

You can instantiate the RandomGenerator class with an explicit seed, or it will seed itself. Under the hood the generator will use the C++ `std::default_random_engine` which may vary depending on the system. 

