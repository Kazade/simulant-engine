#pragma once

#include <simulant/test.h>
#include <fstream>
#include <memory>

namespace {

class FileIfstreamTests : public smlt::test::SimulantTestCase {
public:
    void test_getline() {
        std::string temp_dir = kfs::temp_dir();
        std::string test_file = kfs::path::join(temp_dir, "test.txt");

        std::ofstream fileout(test_file.c_str());
        std::string data = "Line 1\nLine 2\n";
        fileout.write(data.c_str(), data.size());
        fileout.close();

        std::ifstream stream2(test_file);
        assert_true(stream2.good());

        std::string line;
        assert_true(std::getline(stream2, line).good());
        assert_equal(line, "Line 1");

        assert_true(std::getline(stream2, line).good());
        assert_equal(line, "Line 2");

        assert_false(std::getline(stream2, line).good());
        assert_equal(line, "");

        auto buf = std::make_shared<FileStreamBuf>(test_file, "r");
        smlt::FileIfstream stream1(buf);
        assert_true(stream1.good());

        assert_true(std::getline(stream1, line).good());
        assert_equal(line, "Line 1");

        assert_true(std::getline(stream1, line).good());
        assert_equal(line, "Line 2");

        assert_false(std::getline(stream1, line).good());
        assert_equal(line, "");
    }
};

}
