project(Core VERSION 1.0.0 DESCRIPTION "Core static library")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h" "*.hpp")

add_library(${PROJECT_NAME} ${SOURCE_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
	glm
)
