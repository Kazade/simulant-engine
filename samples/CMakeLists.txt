
LINK_LIBRARIES(
    simulant
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/ DESTINATION ${CMAKE_BINARY_DIR}/sample_data/)
FILE(COPY ${CMAKE_SOURCE_DIR}/simulant/particles/ DESTINATION ${CMAKE_BINARY_DIR}/simulant/particles/)

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
ADD_EXECUTABLE(fleets_demo fleets_demo.cpp)
ADD_EXECUTABLE(terrain_demo terrain_sample.cpp)
ADD_EXECUTABLE(physics_demo physics_demo.cpp)
ADD_EXECUTABLE(nehe01 nehe01.cpp)
ADD_EXECUTABLE(ui_demo ui_demo.cpp)

