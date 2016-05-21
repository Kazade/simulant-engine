#ifndef TEST_KAZBASE_H
#define TEST_KAZBASE_H

#include "kaztest/kaztest.h"

#include "kglt/kglt.h"
#include "global.h"

class KazbaseTest : public TestCase {
public:
    void test_unicode_formatting() {
        unicode to_format("{0} {1:.2f}");
        assert_equal(_u("1 3.14"), to_format.format(1, 3.14159));
    }
};


#endif // TEST_KAZBASE_H
