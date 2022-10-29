project(RenderLib VERSION 1.0.0 DESCRIPTION "Suoh Engine Rendering API Library")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h" "*.hpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC
	Core
	Vulkan::Vulkan
	VulkanMemoryAllocator
	glfw
	glslang
	SPIRV
)

# Precompile vulkan.hpp for fast build/compile times
target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_precompile_headers(${PROJECT_NAME} PRIVATE "RenderLib/Vulkan/Headers/vulkan.hpp")

# Ugly temp glslang tbuiltinresource workaround
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/../Vendor/glslang/StandAlone/ResourceLimits.cpp)

