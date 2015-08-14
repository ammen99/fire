cmake_minimum_required(VERSION 3.1.0)
project(animate CXX)

file(GLOB SRC "*.cpp")
add_library(animate SHARED ${SRC})

install(TARGETS   animate    DESTINATION lib/fireman/)
install(DIRECTORY shaders/bg DESTINATION share/fireman/animate)
install(DIRECTORY shaders/pt DESTINATION share/fireman/animate)
