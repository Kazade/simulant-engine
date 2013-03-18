FILE(GLOB_RECURSE TEST_FILES *.h)

FILE(GLOB_RECURSE TEST_SOURCES *.cpp)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    COMMAND ${CMAKE_SOURCE_DIR}/kglt/kazbase/bin/gen_tests.py ${TEST_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    DEPENDS ${TEST_FILES} ${CMAKE_SOURCE_DIR}/kglt/kazbase/bin/gen_tests.py
)

ADD_EXECUTABLE(kglt_tests ${TEST_FILES} ${TEST_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
ADD_TEST(kglt_suite kglt_tests)

TARGET_LINK_LIBRARIES(
    kglt_tests
    kglt
)

