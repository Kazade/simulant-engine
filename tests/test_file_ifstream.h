#pragma once

#include <simulant/test.h>
#include <fstream>
#include <memory>

#include "../simulant/streams/file_ifstream.h"

namespace {

using namespace smlt;

class FileIfstreamTests : public smlt::test::TestCase {
public:
    std::string test_file;

    void set_up() {
        std::string temp_dir = kfs::temp_dir();
        test_file = kfs::path::join(temp_dir, "test.txt");

        std::ofstream fileout(test_file.c_str());
        std::string data = "Line 1\nLine 2\n";
        fileout.write(data.c_str(), data.size());
        fileout.close();
    }

    void test_getline() {
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

    void test_negative_seek() {
        auto buf = std::make_shared<FileStreamBuf>(test_file, "r");
        smlt::FileIfstream stream1(buf);
        assert_true(stream1.good());

        while(stream1.good()) {
            stream1.get();
        }

        assert_true(stream1.eof());
        stream1.clear();

        stream1.seekg(-1, std::ios::cur);
        stream1.seekg(-1, std::ios::cur);
        stream1.seekg(-1, std::ios::cur);

        assert_false(stream1.eof());
        stream1.clear();

        char c = stream1.peek();
        assert_equal(c, ' ');

        c = stream1.get();
        assert_equal(c, ' ');

        c = stream1.get();
        assert_equal(c, '2');

        c = stream1.get();
        assert_equal(c, '\n');
    }

    void test_unget() {
        auto buf = std::make_shared<FileStreamBuf>(test_file, "r");
        smlt::FileIfstream stream1(buf);
        assert_true(stream1.good());

        while(stream1.good()) {
            stream1.get();
        }

        assert_true(stream1.eof());
        stream1.clear();

        stream1.unget();
        stream1.unget();
        stream1.unget();

        assert_false(stream1.eof());
        stream1.clear();

        char c = stream1.peek();
        assert_equal(c, ' ');

        c = stream1.get();
        assert_equal(c, ' ');

        c = stream1.get();
        assert_equal(c, '2');

        c = stream1.get();
        assert_equal(c, '\n');
    }
};

}
