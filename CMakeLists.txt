cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
set(CMAKE_CXX_STANDARD 11)

project(wayfire C CXX)
# Find required package
find_package(PkgConfig)

pkg_check_modules(xlib REQUIRED x11 xext xdamage xfixes xcomposite)
pkg_check_modules(gl REQUIRED gl glew ILUT)

# Main executable
include_directories(include /usr/include/pixman-1 /usr/include/dbus-1.0/ /usr/lib64/dbus-1.0/include src/wlc/include src/wlc/src src/wlc/shared)
file(GLOB SOURCES "src/*.cpp")

add_executable(wayfire ${SOURCES})

# Compiler options
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -fpermissive")

# Libraries
target_link_libraries(wayfire X11 Xext Xdamage Xfixes Xcomposite)
target_link_libraries(wayfire jpeg GLESv2 GL GLEW IL ILU ILUT)
target_link_libraries(wayfire dl wayland-server wlc xkbcommon input wayland-client wayland-cursor wayland-egl)

# Installation
install(TARGETS wayfire DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/wayfire)
install(DIRECTORY shaders/ DESTINATION share/wayfire/shaders)
install(FILES config DESTINATION lib/wayfire/firerc)

# Plugins
add_subdirectory(plugins)

