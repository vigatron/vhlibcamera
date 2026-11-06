cmake_minimum_required(VERSION 3.16)

add_executable(test_vhwdgcam
    test_vhwdgcam.cpp
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_link_libraries(test_vhwdgcam PRIVATE
    vhlib_camera
    Qt6::Core
    Qt6::Widgets
)
