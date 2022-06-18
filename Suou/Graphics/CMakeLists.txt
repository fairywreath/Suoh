project(graphics-lib VERSION 1.0.0 DESCRIPTION "Suou engine graphics library")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE GRAPHICS_LIB_SOURCES "*.cpp" "*.h")

add_library(${PROJECT_NAME} ${GRAPHICS_LIB_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC
    core
    platform
    type_safe
    Vulkan::Vulkan
    VulkanMemoryAllocator
    Vulkan::Headers
)