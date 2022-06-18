if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

project(suou VERSION 1.0.0)

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(Suou)
