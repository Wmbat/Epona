# CMake project initialization

cmake_minimum_required( VERSION 3.14 FATAL_ERROR )

# Download CPM.cmake

set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
set(CPM_DOWNLOAD_VERSION 0.27)

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION} AND CPM_VERSION STREQUAL CPM_DOWNLOAD_VERSION))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

# Set the project language toolchain, version and description

project(vermillon
    VERSION 0.0.1
    DESCRIPTION "A Vulkan based rendering engine"
    LANGUAGES CXX
)

# User interface declarations

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCH "Build benchmarks" ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "[${PROJECT_NAME}] Compiling with ${CMAKE_CXX_COMPILER_ID}")

message(STATUS "[${PROJECT_NAME}] ${PROJECT_VERSION}")

if (CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "[${PROJECT_NAME}] Build type: Release")
else ()
    message(STATUS "[${PROJECT_NAME}] Build type: Debug")
endif ()

if (BUILD_TESTS)
   set(VERMILLON_CORE_BUILD_TESTS ON CACHE BOOL "" FORCE)
   set(VERMILLON_UTIL_BUILD_TESTS ON CACHE BOOL "" FORCE)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(VERMILLON_CORE_STATIC ON CACHE BOOL "" FORCE)

message(STATUS "[${PROJECT_NAME}] Building unit tests: ${BUILD_TESTS}")
message(STATUS "[${PROJECT_NAME}] Building benchmarks: ${BUILD_BENCH}")

add_subdirectory(modules/core)
add_subdirectory(modules/gfx)
add_subdirectory(modules/util)
add_subdirectory(modules/vkn)

add_executable(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        VERMILLON_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        VERMILLON_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        VERMILLON_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)


target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<PLATFORM_ID:UNIX>:-pthread>

        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:DEBUG>>:-o0 -g -Wall -Wextra -Werror -fno-omit-frame-pointer 
        -fsanitize=address>
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:RELEASE>>:-o3>

        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:-o0 -g -Wall -Wextra -Werror -fno-omit-frame-pointer
        -Wconversion -fsanitize=address, -fsanitize=undefined>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:-o3>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        vermillon::core
        vermillon::gfx
        vermillon::util
        vermillon::vkn

    PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:DEBUG>>:-lasan>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:-lasan>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:-lubsan>
)

target_sources(${PROJECT_NAME}
   PRIVATE
      source/main.cpp
)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources)
