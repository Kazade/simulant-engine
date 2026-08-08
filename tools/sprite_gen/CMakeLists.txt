LINK_LIBRARIES(
    simulant
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(sprite_gen
    main.cpp
    sprite_gen_scene.cpp
    tga_writer.cpp
    atlas_writer.cpp
)

install(TARGETS sprite_gen RUNTIME DESTINATION bin)
