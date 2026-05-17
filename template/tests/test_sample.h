#pragma once

#include "simulant/test.h"

namespace {

using namespace smlt;

class SimpleTestCase : public test::SimulantTestCase {
public:
  void test_addition() {
    auto add = [](int a, int b) {
      return a + b;
    };

    assert_equal(4, add(2, 2));
  }
};

}
