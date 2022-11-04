project(SceneConverter VERSION 1.0.0 DESCRIPTION "Scene converter is an EXECUTABLE_OUTPUT_PATH")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# dynamically link assimp
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../Vendor/assimp/out/Install/x64-Release/lib/assimp-vc143-mt.lib)

target_include_directories(${PROJECT_NAME} PUBLIC 
	${PROJECT_SOURCE_DIR}/../Vendor/assimp/out/Install/x64-Release/include
	${PROJECT_SOURCE_DIR}/../Vendor/stb_image 
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	RenderDescription
	meshoptimizer
)
