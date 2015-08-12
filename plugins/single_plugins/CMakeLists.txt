cmake_minimum_required(VERSION 3.1.0)
project(simple_plugins CXX)

add_library(expo     SHARED "expo.cpp")
add_library(grid     SHARED "grid.cpp")
add_library(move     SHARED "move.cpp")
add_library(resize   SHARED "resize.cpp")
add_library(switcher SHARED "switcher.cpp")
add_library(vswitch  SHARED "vswitch.cpp")
add_library(command  SHARED "command.cpp")

install(TARGETS expo     DESTINATION lib/fireman/)
install(TARGETS grid     DESTINATION lib/fireman/)
install(TARGETS move     DESTINATION lib/fireman/)
install(TARGETS resize   DESTINATION lib/fireman/)
install(TARGETS switcher DESTINATION lib/fireman/)
install(TARGETS vswitch  DESTINATION lib/fireman/)
install(TARGETS command  DESTINATION lib/fireman/)

