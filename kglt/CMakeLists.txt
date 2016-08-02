FILE(GLOB_RECURSE KGLT_HEADERS *h)
FILE(GLOB_RECURSE KGLT_FILES FOLLOW_SYMLINKS *.cpp *.c *.cc)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)

ADD_LIBRARY(kglt SHARED ${KGLT_FILES} ${DEPS_SOURCES})
SET_TARGET_PROPERTIES(kglt
                      PROPERTIES
                      VERSION 0.0.1
                      SOVERSION 1)

INSTALL(TARGETS kglt DESTINATION lib)
INSTALL(DIRECTORY ../deps DESTINATION include/kglt FILES_MATCHING PATTERN *.h)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/kglt-config.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/opengl-2.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials/opengl-2.x)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/opengl-1.x/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials/opengl-1.x)

FILE(GLOB textures "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.png")
INSTALL(FILES ${textures} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.vert")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB_RECURSE materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.frag")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB particles "${CMAKE_CURRENT_SOURCE_DIR}/particles/*.kglp")
INSTALL(FILES ${particles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/particles)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/ubuntu/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/ubuntu)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/opensans/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/opensans)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/fontawesome/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/fontawesome)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/probe/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/probe)
