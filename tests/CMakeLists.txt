FILE(GLOB_RECURSE TEST_FILES *.cpp)

ADD_EXECUTABLE(kglt_tests ${TEST_FILES})
ADD_TEST(kglt_suite kglt_tests)
