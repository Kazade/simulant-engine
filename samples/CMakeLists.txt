
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
ADD_EXECUTABLE(parallax_sample parallax_sample.cpp)
ADD_EXECUTABLE(combined_sample_1 combined_sample_1.cpp)
ADD_EXECUTABLE(ui_sample ui_sample.cpp)
ADD_EXECUTABLE(text_sample text_sample.cpp)
ADD_EXECUTABLE(light_sample light_sample.cpp)
ADD_EXECUTABLE(texture_scroll_sample texture_scroll_sample.cpp)
