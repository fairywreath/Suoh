project(platform VERSION 1.0.0 DESCRIPTION "Suou platform-specific library")

file(GLOB_RECURSE PLATFORM_SOURCES "*.cpp" "*.h")

add_library(${PROJECT_NAME} ${PLATFORM_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC 
    core
    glfw
)
