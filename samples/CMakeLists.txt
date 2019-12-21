
LINK_LIBRARIES(
    simulant
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(sample sample.cpp)
ADD_EXECUTABLE(2d_sample 2d_sample.cpp)
ADD_EXECUTABLE(viewport_sample viewport_sample.cpp)
ADD_EXECUTABLE(selection_sample selection_sample.cpp)
ADD_EXECUTABLE(light_sample light_sample.cpp)
ADD_EXECUTABLE(rtt_sample rtt_sample.cpp)
ADD_EXECUTABLE(q2bsp_sample q2bsp_sample.cpp)
ADD_EXECUTABLE(fleets_demo fleets_demo.cpp)
ADD_EXECUTABLE(terrain_demo terrain_sample.cpp)
ADD_EXECUTABLE(physics_demo physics_demo.cpp)
ADD_EXECUTABLE(nehe02 nehe02.cpp)
ADD_EXECUTABLE(nehe03 nehe03.cpp)
ADD_EXECUTABLE(ui_demo ui_demo.cpp)
ADD_EXECUTABLE(particles particles.cpp)


SET(SAMPLES
    sample
    2d_sample
    viewport_sample
    selection_sample
    light_sample
    rtt_sample
    q2bsp_sample
    fleets_demo
    terrain_demo
    physics_demo
    nehe02
    nehe03
    ui_demo
    particles
)

if(SIMULANT_SEPERATE_DEBUGINFO)
foreach(sample ${SAMPLES})
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${sample}.debug
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${CMAKE_CURRENT_BINARY_DIR}/${sample} ${CMAKE_CURRENT_BINARY_DIR}/${sample}.debug && ${CMAKE_OBJCOPY} --strip-debug ${CMAKE_CURRENT_BINARY_DIR}/${sample}
    DEPENDS ${sample}
    COMMENT "Separating debug info for ${sample}"
)
add_custom_target(${sample}.debug.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${sample}.debug)
endforeach()
endif()
