

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies
)

LINK_LIBRARIES(
    kglt
    ${KAZMATH_LIBRARIES}
)

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/ DESTINATION ${CMAKE_BINARY_DIR}/sample_data/)
ADD_EXECUTABLE(sample sample.cpp)
ADD_EXECUTABLE(2d_sample 2d_sample.cpp)
ADD_EXECUTABLE(viewport_sample viewport_sample.cpp)
ADD_EXECUTABLE(selection_sample selection_sample.cpp)
ADD_EXECUTABLE(combined_sample_1 combined_sample_1.cpp)
ADD_EXECUTABLE(light_sample light_sample.cpp)
ADD_EXECUTABLE(flightsim flightsim.cpp)
ADD_EXECUTABLE(joypad_sample joypad_sample.cpp)
ADD_EXECUTABLE(rtt_sample rtt_sample.cpp)
ADD_EXECUTABLE(q2bsp_sample q2bsp_sample.cpp)

