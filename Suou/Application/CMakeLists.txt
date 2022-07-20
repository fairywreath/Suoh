project(suou-appplication VERSION 1.0.0 DESCRIPTION "Suou application")

file(GLOB_RECURSE CLIENT_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${CLIENT_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC
	core
	render-lib
	platform
)

target_precompile_headers(${PROJECT_NAME} PRIVATE "suoupch.h")
