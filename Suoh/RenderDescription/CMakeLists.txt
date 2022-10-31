project(RenderDescription VERSION 1.0.0 DESCRIPTION "Suoh Engine Rendering <PROJECT-NAME>_DESCRIPTION")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h" "*.hpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC
	Core
)

