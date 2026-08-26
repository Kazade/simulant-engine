LINK_LIBRARIES(
    simulant
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(sprite_gen
    main.cpp
    sprite_gen_scene.cpp
    tga_writer.cpp
    dtex_writer.cpp
    png_writer.cpp
    atlas_writer.cpp
)

# Location of the texconv tool, used at runtime by -C/--compress. Can be
# overridden with -DTEXCONV=/path/to/texconv
find_program(TEXCONV texconv)
if(TEXCONV)
    target_compile_definitions(sprite_gen PRIVATE DEFAULT_TEXCONV="${TEXCONV}")
endif()

install(TARGETS sprite_gen RUNTIME DESTINATION bin)
