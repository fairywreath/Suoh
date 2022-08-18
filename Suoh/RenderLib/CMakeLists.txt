project(render-lib VERSION 1.0.0 DESCRIPTION "Suoh engine rendering library")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE RENDER_LIB_SOURCES "*.cpp" "*.h")

add_library(${PROJECT_NAME} ${RENDER_LIB_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC
    core
    platform
    type_safe
    Vulkan::Vulkan
    VulkanMemoryAllocator
    Vulkan::Headers
    glslang
    SPIRV
    stb_image
    assimp
)


# manually add DefaultTBuiltinresource....., maybe initialize this inside this project?
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/../Vendor/glslang/StandAlone/ResourceLimits.cpp)