project(meshconvert VERSION 1.0.0 DESCRIPTION "Mesh conversion executable")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE
	core
	graphics
	meshoptimizer
	# OptickCore
)
