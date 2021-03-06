# this will make sure that one is using the clang compiler
set(CMAKE_CXX_COMPILER  "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")

project(gecko-http)

set(CMAKE_BINARY_DIR          ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH    ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_TEMP_OUTPUT_DIR     ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=c11 -Wall -Werror")

# include third-party cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

cmake_minimum_required(VERSION 3.5)

include(
        "${CMAKE_SOURCE_DIR}/cmake/FindAPR.cmake"
)

find_package(APR     REQUIRED)

option(BUILD_DOCUMENTATION "API documentation requires Doxygen" ${DOXYGEN_FOUND})

# include third-party cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR})

message("APR include: " ${APR_INCLUDE_DIR})


include_directories(
        "include"
        "libs/gecko-commons/include"

        ${APR_INCLUDE_DIR}
        # This is a hack in case APR is not found in macOS High Sierra.
        # You may install apr and apr-utils from sources and symbolic link these directories
        "/usr/local/apr/include/apr-1/"
        "/usr/local/apr/include/apr-utils"

)

file(GLOB_RECURSE COMMONS_SOURCE_FILES "libs/gecko-commons/src/*.c")

set(SOURCE_FILES
        ${COMMONS_SOURCE_FILES}
        include/gecko-http/gecko-http.h
        include/gecko-http/gs_http.h
        include/gecko-http/gs_request.h
        include/gecko-http/gs_response.h
        include/gecko-http/gs_server.h
        include/gecko-http/gs_stdinc.h
        include/gecko-http/gs_event.h

        src/gecko-http/gs_http.c
        src/gecko-http/gs_request.c
        src/gecko-http/gs_response.c
        src/gecko-http/gs_server.c
        src/gecko-http/gs_event.c

        include/gecko-http/gecko-http.h
        include/gecko-http/gs_stdinc.h

        include/gecko-http/gecko-http.h)

add_executable(
    gecko
    test.c
    ${SOURCE_FILES}
)

target_link_libraries(
        gecko
        ${APR_LIBRARIES}
        ${APRUTIL_LIBRARIES}
)

if(DOXYGEN_FOUND)
    add_custom_target(
        doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)