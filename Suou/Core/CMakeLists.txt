project(core VERSION 1.0.0 DESCRIPTION "Core static library")

file(GLOB_RECURSE CORE_SOURCES "*.cpp" "*.h")

add_library(${PROJECT_NAME} ${CORE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME} PUBLIC
    type_safe
    glm
)
