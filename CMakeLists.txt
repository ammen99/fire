cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_CXX_STANDARD 11)
project(fireman CXX)


# Find required packages
find_package(PkgConfig)

pkg_check_modules(xlib REQUIRED x11 xext xdamage xfixes xcomposite)
pkg_check_modules(gl REQUIRED gl glew ILUT)


# Main executable
include_directories(include)
file(GLOB SOURCES "src/*.cpp")
add_executable(fireman ${SOURCES})

# Libraries
target_link_libraries(fireman X11 Xext Xdamage Xfixes Xcomposite)
target_link_libraries(fireman GL GLEW IL ILU ILUT)
target_link_libraries(fireman dl)

# Installation
install(TARGETS fireman DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/fireman)
install(DIRECTORY shaders/ DESTINATION share/fireman/shaders)

# Plugins
add_subdirectory(plugins)
