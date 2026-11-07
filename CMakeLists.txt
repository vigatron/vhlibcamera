cmake_minimum_required(VERSION 3.16)

project(vhlibcamera VERSION 0.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MYLIB_BUILD_TESTS "Build test applications" OFF)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version_vhlibcamera.h"
)

# Настройки для Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Multimedia)


set(VHLIB_CAMERA_SOURCES
    src/vhwdgcamimg.hpp
    src/vhwdgcamimg.cpp
    src/vhwdgcam.hpp
    src/vhwdgcam.cpp
)

add_library(vhlib_camera STATIC 
    ${VHLIB_CAMERA_SOURCES}
)

target_include_directories(vhlib_camera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# target_compile_definitions(vhlib_camera PUBLIC _FILE_OFFSET_BITS=64)

target_link_libraries(vhlib_camera PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
)

enable_testing()

if(MYLIB_BUILD_TESTS)
    add_subdirectory(test)
endif()
