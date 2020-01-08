CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(RedisHDR)

# Set a default build type if none was specified
set(default_build_type "Release")

IF(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()

option(USE_COVERAGE "Test for code coverage" OFF)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# For adding specific Release flags
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Add -fno-omit-frame-pointer to avoid seeing incomplete stack traces
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb -fno-omit-frame-pointer")

find_package( ZLIB REQUIRED )

#----------------------------------------------------------------------------------------------

FUNCTION(ADD_LDFLAGS _TARGET NEW_FLAGS)
    GET_TARGET_PROPERTY(LD_FLAGS ${_TARGET} LINK_FLAGS)
    IF(LD_FLAGS)
        SET(NEW_FLAGS "${LD_FLAGS} ${NEW_FLAGS}")
    ENDIF()
    SET_TARGET_PROPERTIES(${_TARGET} PROPERTIES LINK_FLAGS ${NEW_FLAGS})
ENDFUNCTION()

MACRO(install_symlink filepath sympath)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})")
    install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
ENDMACRO(install_symlink)

SET(CMAKE_C_STANDARD 11)
ADD_DEFINITIONS(-DREDISMODULE_EXPERIMENTAL_API)

#----------------------------------------------------------------------------------------------

ADD_SUBDIRECTORY(src)
ADD_LIBRARY(redis_hdr SHARED $<TARGET_OBJECTS:redis_hdr_obj>)

if ( ZLIB_FOUND )
    include_directories( ${ZLIB_INCLUDE_DIRS} )
    target_link_libraries( redis_hdr ${ZLIB_LIBRARIES} )
endif( ZLIB_FOUND )

SET_TARGET_PROPERTIES(redis_hdr PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(redis_hdr PROPERTIES SUFFIX ".so")

IF (APPLE)
    SET_TARGET_PROPERTIES(redis_hdr PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup")
ENDIF()

SET(CMAKE_INSTALL_PREFIX ${installAbs})

INSTALL(TARGETS redis_hdr LIBRARY DESTINATION .)

IF (APPLE)
    SET_TARGET_PROPERTIES(redis_hdr PROPERTIES INSTALL_RPATH "@loader_path/lib")
ELSE ()
    ADD_LDFLAGS(redis_hdr "-Wl,--enable-new-dtags")
    SET_TARGET_PROPERTIES(redis_hdr PROPERTIES INSTALL_RPATH "\$ORIGIN/lib")
ENDIF()

IF (APPLE)
    SET(LIB_PATTERN "*.dylib")
ELSE()
    SET(LIB_PATTERN "*.so*")
ENDIF()


IF (USE_COVERAGE)
    SET(_lcovInit "${PROJECT_BINARY_DIR}/lcov-init.sh")
    SET(_lcovCapture "${PROJECT_BINARY_DIR}/lcov-capture.sh")
    CONFIGURE_FILE("cmake/lcov-init.sh.in" "${_lcovInit}" @ONLY)
    CONFIGURE_FILE("cmake/lcov-capture.sh.in" "${_lcovCapture}" @ONLY)
    EXECUTE_PROCESS(COMMAND chmod a+x ${_lcovInit} ${_lcovCapture})
ENDIF()
