cmake_minimum_required(VERSION 3.0.0)

project(SuohEngine VERSION 0.2.0)

find_package(Vulkan REQUIRED)

add_subdirectory(Source)

if(NOT WIN32)
    message("Error - Current OS not supported!")
endif()