
LINK_LIBRARIES(
    kglt
    ${KAZMATH_LIBRARIES}
)

FILE(COPY ${CMAKE_SOURCE_DIR}/sample_data DESTINATION ${CMAKE_BINARY_DIR})
ADD_EXECUTABLE(sample sample.cpp)
ADD_EXECUTABLE(q2_sample q2bsp_sample.cpp)
ADD_EXECUTABLE(2d_sample 2d_sample.cpp)
ADD_EXECUTABLE(viewport_sample viewport_sample.cpp)
ADD_EXECUTABLE(selection_sample selection_sample.cpp)
ADD_EXECUTABLE(outline_sample outline_sample.cpp)
