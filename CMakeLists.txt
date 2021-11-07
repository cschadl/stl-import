cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "stl_import_project")
project(${PROJECT_NAME} VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(STL_IMPORT_LIB "stl_import")
set(STL_IMPORT_TESTS "stl_import_tests")

option(BUILD_STATIC "Build static library" OFF)
option(BUILD_TESTS "Build unit tests" ON)
set(MATHSTUFF_PATH ${CMAKE_SOURCE_DIR}/submodules/mathstuff CACHE STRING "path to mathstuff")
set(STLUTIL_PATH ${CMAKE_SOURCE_DIR}/submodules/stlutil CACHE STRING "path to stlutil")

set(STL_IMPORT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/stl_import)

file(GLOB_RECURSE STL_IMPORT_H ${STL_IMPORT_INCLUDE_DIR}/*.h)
file(GLOB_RECURSE STL_IMPORT_SRC ${STL_IMPORT_INCLUDE_DIR}/*.cpp)
file(GLOB_RECURSE STL_IMPORT_TESTS_SRC ${CMAKE_SOURCE_DIR}/tests/*.cpp)

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
    target_link_libraries(${STL_IMPORT_TESTS} PRIVATE ${STL_IMPORT_LIB})

    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${MATHSTUFF_PATH})
    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${STLUTIL_PATH})
    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${EIGEN3_INCLUDE_DIR})
    target_include_directories(${STL_IMPORT_TESTS} PUBLIC ${STL_IMPORT_INCLUDE_DIR})
endif(BUILD_TESTS)

add_custom_target(copy-test-data
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/test_data ${CMAKE_BINARY_DIR}/test_data
    DEPENDS ${STL_IMPORRT_TESTS}
)

install (TARGETS ${STL_IMPORT_LIB}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION inc)