project(suoh-appplication VERSION 1.0.0 DESCRIPTION "Suoh application")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE
	core
	graphics
	imgui
	# XXX: make this adjustable from cli
	# OptickCore
)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "")

# target_precompile_headers(${PROJECT_NAME} PRIVATE "Application/suohpch.h")
