#pragma once

#include "simulant/test.h"

namespace {

using namespace smlt;

class StreamTests : public smlt::test::SimulantTestCase {
public:
    void test_pipe_memory_into_string() {
        std::string out;

        auto memory = open(":memory:");
        memory->write("Hello World!");

        read_into(memory, out);
    }

    void test_pipe_file_into_string() {
        std::string out;
        auto input = open("cube.obj", STREAM_MODE_READ);
        read_into(input, out);
    }
};

}
