project(suoh-appplication VERSION 1.0.0 DESCRIPTION "Suoh application")

file(GLOB_RECURSE CLIENT_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${CLIENT_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} PUBLIC
	core
	render-lib
	platform
	imgui
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "suohpch.h")
