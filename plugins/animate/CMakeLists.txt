cmake_minimum_required(VERSION 3.1.0)
project(animate CXX)

add_library(animate SHARED "animate.cpp" "fade.cpp")

install(TARGETS animate DESTINATION lib/fireman/)
