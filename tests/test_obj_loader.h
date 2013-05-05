#ifndef TEST_OBJ_LOADER_H
#define TEST_OBJ_LOADER_H

#include "kglt/loaders/obj_loader.h"

class OBJLoaderTest : public TestCase {
public:
    void test_face_parsing() {
        unicode line = "1//2";

        int32_t v, vn, vt;

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(1, vn);

        line = "1";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(-1, vn);

        line = "1/2";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(1, vt);
        assert_equal(-1, vn);

        line = "1/2/3";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(1, vt);
        assert_equal(2, vn);

        line = "1//";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(-1, vn);
    }
};

#endif // TEST_OBJ_LOADER_H
