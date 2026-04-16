# Install script for directory: /home/kazade/Projects/simulant/deps/sh4zam

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/opt/toolchains/dc/sh-elf/bin/sh-elf-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/kazade/Projects/simulant/dbuildr/deps/sh4zam/libsh4zam.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/sh4zam/shz_cdefs.h;/sh4zam/shz_cdefs.hpp;/sh4zam/shz_circle.h;/sh4zam/shz_circle.hpp;/sh4zam/shz_trig.h;/sh4zam/shz_trig.hpp;/sh4zam/shz_scalar.h;/sh4zam/shz_scalar.hpp;/sh4zam/shz_vector.h;/sh4zam/shz_vector.hpp;/sh4zam/shz_xmtrx.h;/sh4zam/shz_xmtrx.hpp;/sh4zam/shz_matrix.h;/sh4zam/shz_matrix.hpp;/sh4zam/shz_quat.h;/sh4zam/shz_quat.hpp;/sh4zam/shz_mem.h;/sh4zam/shz_mem.hpp;/sh4zam/shz_sh4zam.h;/sh4zam/shz_sh4zam.hpp;/sh4zam/shz_trig.inl.h;/sh4zam/shz_mem.inl.h;/sh4zam/shz_quat.inl.h;/sh4zam/shz_matrix.inl.h;/sh4zam/shz_vector.inl.h;/sh4zam/shz_scalar.inl.h;/sh4zam/shz_xmtrx.inl.h;/sh4zam/shz_mem_sh4.inl.h;/sh4zam/shz_quat_sh4.inl.h;/sh4zam/shz_scalar_sh4.inl.h;/sh4zam/shz_trig_sh4.inl.h;/sh4zam/shz_vector_sh4.inl.h;/sh4zam/shz_xmtrx_sh4.inl.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/sh4zam" TYPE FILE FILES
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_cdefs.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_cdefs.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_circle.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_circle.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_trig.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_trig.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_scalar.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_scalar.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_vector.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_vector.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_xmtrx.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_xmtrx.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_matrix.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_matrix.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_quat.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_quat.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_mem.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_mem.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_sh4zam.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/shz_sh4zam.hpp"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_trig.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_mem.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_quat.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_matrix.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_vector.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_scalar.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/shz_xmtrx.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/sh4/shz_mem_sh4.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/sh4/shz_quat_sh4.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/sh4/shz_scalar_sh4.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/sh4/shz_trig_sh4.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/sh4/shz_vector_sh4.inl.h"
    "/home/kazade/Projects/simulant/deps/sh4zam/include/sh4zam/inline/sh4/shz_xmtrx_sh4.inl.h"
    )
endif()

