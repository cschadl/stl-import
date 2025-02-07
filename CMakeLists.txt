cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "stl_import_project")
project(${PROJECT_NAME} VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(STL_IMPORT_LIB "stl_import")
set(STL_IMPORT_TESTS "stl_import_tests")

if (NOT STLIMPORT_PATH)
    set(STLIMPORT_PATH ${CMAKE_SOURCE_DIR})
endif(NOT STLIMPORT_PATH)

option(BUILD_STATIC "Build static library" OFF)
option(BUILD_TESTS "Build unit tests" ON)
set(MATHSTUFF_PATH ${STLIMPORT_PATH}/submodules/mathstuff CACHE STRING "path to mathstuff")
set(STLUTIL_PATH ${STLIMPORT_PATH}/submodules/stlutil CACHE STRING "path to stlutil")

set(STL_IMPORT_INCLUDE_DIR ${STLIMPORT_PATH}/stl_import)

file(GLOB_RECURSE STL_IMPORT_H ${STL_IMPORT_INCLUDE_DIR}/*.h)
file(GLOB_RECURSE STL_IMPORT_SRC ${STL_IMPORT_INCLUDE_DIR}/*.cpp)
file(GLOB_RECURSE STL_IMPORT_TESTS_SRC ${STLIMPORT_PATH}/tests/*.cpp)

find_package(Eigen3 REQUIRED)

if (NOT BUILD_STATIC)
    add_library(${STL_IMPORT_LIB} SHARED ${STL_IMPORT_H} ${STL_IMPORT_SRC})
else(NOT BUILD_STATIC)
    add_library(${STL_IMPORT_LIB} STATIC ${STL_IMPORT_H} ${STL_IMPORT_SRC})
endif(NOT BUILD_STATIC)

target_include_directories(${STL_IMPORT_LIB} PUBLIC ${MATHSTUFF_PATH})
target_include_directories(${STL_IMPORT_LIB} PUBLIC ${STLUTIL_PATH})
target_include_directories(${STL_IMPORT_LIB} PUBLIC ${EIGEN3_INCLUDE_DIR})

set_target_properties(${STL_IMPORT_LIB} PROPERTIES PUBLIC_HEADER "${STL_IMPORT_H}")

if (BUILD_TESTS)
    add_executable(${STL_IMPORT_TESTS} ${STL_IMPORT_TESTS_SRC})
    target_link_libraries(${STL_IMPORT_TESTS} PUBLIC ${STL_IMPORT_LIB})

    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${MATHSTUFF_PATH})
    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${STLUTIL_PATH})
    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${EIGEN3_INCLUDE_DIR})
    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${STL_IMPORT_INCLUDE_DIR})
endif(BUILD_TESTS)

if (BUILD_TESTS)
    add_custom_target(copy-test-data
        ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${STLIMPORT_PATH}/tests/test_data ${CMAKE_BINARY_DIR}/test_data
        DEPENDS ${STL_IMPORT_TESTS}
    )
endif(BUILD_TESTS)

if (NOT BUILD_STATIC)
    install (TARGETS ${STL_IMPORT_LIB}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION inc)
endif(NOT BUILD_STATIC)