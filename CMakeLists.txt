message("[VUGDEBUG] CMake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.29)

message("[VUGDEBUG] Compilers.\n C: ${CMAKE_C_COMPILER}\n CPP: ${CMAKE_CXX_COMPILER}\n CUDA: ${CMAKE_CUDA_COMPILER}")

project(GraphicsWorkshop
  VERSION 0.1
  DESCRIPTION "Some 3rd party graphics libraries and some custom functionalities on top of them, so that I won't need to start a new repo from scratch each time I study a topic in computer graphics."
  LANGUAGES CXX)

add_subdirectory(third-party)
add_subdirectory(workshop)