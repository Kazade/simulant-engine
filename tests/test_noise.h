#pragma once

#include <cmath>
#include <simulant/test.h>
#include <simulant/utils/noise.h>

namespace {

using namespace smlt;
using namespace smlt::noise;

class FractalNoiseTests: public smlt::test::SimulantTestCase {
public:
    void test_output_is_normalized() {
        for(int octaves = 1; octaves <= 8; ++octaves) {
            FractalNoise noise(octaves, 1234);

            for(int i = 0; i < 2000; ++i) {
                double x = i * 0.137;
                double y = i * 0.071;
                double z = i * 0.029;

                float n = noise.noise(x, y, z);
                assert_true(n >= -1.0f && n <= 1.0f);

                float nn = noise.normalized_noise(x, y, z);
                assert_true(nn >= 0.0f && nn <= 1.0f);
            }
        }
    }

    void test_normalized_noise_is_const() {
        const FractalNoise noise(4, 42);
        float n = noise.normalized_noise(0.5, 0.5);
        assert_true(n >= 0.0f && n <= 1.0f);
    }

    void test_origin_is_not_zero() {
        // Without per-octave offsets every lattice point (including the
        // origin) evaluates to exactly 0 in every octave
        FractalNoise noise(4, 1234);
        assert_true(std::abs(noise.noise(0.0, 0.0, 0.0)) > 1e-6f);
    }

    void test_same_seed_is_deterministic() {
        FractalNoise a(5, 99);
        FractalNoise b(5, 99);

        for(int i = 0; i < 100; ++i) {
            assert_equal(a.noise(i * 0.3, i * 0.7), b.noise(i * 0.3, i * 0.7));
        }
    }

    void test_custom_parameters() {
        FractalNoise noise(6, 7, 3.0f, 0.25f);
        assert_equal(noise.octaves(), 6);
        assert_close(noise.lacunarity(), 3.0f, 1e-6f);
        assert_close(noise.persistence(), 0.25f, 1e-6f);

        for(int i = 0; i < 500; ++i) {
            float n = noise.noise(i * 0.11, i * 0.23);
            assert_true(n >= -1.0f && n <= 1.0f);
        }
    }

    void test_single_octave_matches_range_of_perlin() {
        FractalNoise noise(1, 5);
        for(int i = 0; i < 500; ++i) {
            float n = noise.noise(i * 0.19, i * 0.41, i * 0.05);
            assert_true(n >= -1.0f && n <= 1.0f);
        }
    }
};

}
