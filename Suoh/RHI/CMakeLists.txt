project(RHI VERSION 1.0.0 DESCRIPTION "Suoh Engine Rendering Hardware INTERFACE_AUTOUIC_OPTIONS")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h" "*.hpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Public)

target_link_libraries(${PROJECT_NAME} PUBLIC
	Core
	Vulkan::Vulkan
	VulkanMemoryAllocator
	glfw
)

# Precompile vulkan.hpp for fast build/compile times
target_compile_definitions(${PROJECT_NAME} PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_precompile_headers(${PROJECT_NAME} PRIVATE "Private/Vulkan/Headers/vulkan.hpp")
