SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   $ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER $ENV{KOS_CC_BASE}/bin/$ENV{KOS_CC_PREFIX}-g++)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  $ENV{KOS_CC_BASE})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(TOOLCHAIN_DEPS_DIR ${CMAKE_BINARY_DIR}/deps)

if(NOT EXISTS ${TOOLCHAIN_DEPS_DIR})
    # Gather and build the dependencies appropriately
    EXECUTE_PROCESS(
        COMMAND "python" "${CMAKE_CURRENT_SOURCE_DIR}/bin/gather_deps.py" "${CMAKE_TOOLCHAIN_FILE}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
ENDIF()

# Add the ports and deps to the search paths
SET(CMAKE_PREFIX_PATH $ENV{KOS_PORTS} ${CMAKE_BINARY_DIR}/deps)
