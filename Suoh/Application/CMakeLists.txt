project(application VERSION 1.0.0 DESCRIPTION "Suoh application")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}/../Vendor/stb_image 
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	RenderLib
	RenderDescription
	gli
)

# set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "")
set_property(TARGET application PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:application>)
