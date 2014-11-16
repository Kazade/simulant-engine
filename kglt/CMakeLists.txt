FILE(GLOB_RECURSE KGLT_HEADERS *h)
FILE(GLOB_RECURSE KGLT_FILES FOLLOW_SYMLINKS *.cpp *.c *.cc)

FILE(GLOB TO_REMOVE "ui/rocket/Source/Core/Lua/*")
LIST(REMOVE_ITEM KGLT_FILES ${TO_REMOVE})

FILE(GLOB TO_REMOVE "ui/rocket/Source/Controls/Lua/*")
LIST(REMOVE_ITEM KGLT_FILES ${TO_REMOVE})

FILE(GLOB TO_REMOVE "ui/rocket/Source/Core/Python/*")
LIST(REMOVE_ITEM KGLT_FILES ${TO_REMOVE})

FILE(GLOB TO_REMOVE "ui/rocket/Source/Controls/Python/*")
LIST(REMOVE_ITEM KGLT_FILES ${TO_REMOVE})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)

ADD_LIBRARY(kglt SHARED ${KGLT_FILES})
SET_TARGET_PROPERTIES(kglt
                      PROPERTIES
                      VERSION 0.0.1
                      SOVERSION 1)
#COTIRE(kglt)

INSTALL(TARGETS kglt DESTINATION lib)
INSTALL(DIRECTORY ./ DESTINATION include/kglt FILES_MATCHING PATTERN *.h PATTERN "ui/rocket*" EXCLUDE PATTERN "lua/luabind*" EXCLUDE)
INSTALL(DIRECTORY ./ DESTINATION include/kglt FILES_MATCHING PATTERN *.hpp PATTERN "ui/rocket*" EXCLUDE PATTERN "lua/luabind*" EXCLUDE)
INSTALL(DIRECTORY ../submodules/luabind/luabind DESTINATION include/kglt/lua FILES_MATCHING PATTERN *.hpp)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/KGLTConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt)

FILE(GLOB materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.kglm")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB textures "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.png")
INSTALL(FILES ${textures} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.vert")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB materials "${CMAKE_CURRENT_SOURCE_DIR}/materials/*.frag")
INSTALL(FILES ${materials} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/materials)

FILE(GLOB particles "${CMAKE_CURRENT_SOURCE_DIR}/particles/*.kglp")
INSTALL(FILES ${particles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/particles)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/ubuntu/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/ubuntu)

FILE(GLOB fonts "${CMAKE_CURRENT_SOURCE_DIR}/fonts/opensans/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/kglt/fonts/opensans)
