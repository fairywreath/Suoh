project(graphics VERSION 1.0.0 DESCRIPTION "Suoh engine graphics library")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h" "*.hpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# dynamically link assimp
# find_package(assimp)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../Vendor/lib/assimp-vc143-mt.lib)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../Vendor/assimp/include)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC
    core
    type_safe
    Vulkan::Vulkan
    VulkanMemoryAllocator
    Vulkan::Headers
    glslang
    SPIRV
    stb_image
    glfw
    # assimp
)


# manually add DefaultTBuiltinresource....., maybe initialize this inside this project?
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/../Vendor/glslang/StandAlone/ResourceLimits.cpp)