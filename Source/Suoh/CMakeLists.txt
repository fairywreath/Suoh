project(SuohEngine VERSION 1.0.0 DESCRIPTION "Suoh Engine")

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    # VULKAN_HPP_NO_EXCEPTIONS=1
    VULKAN_HPP_NO_SMART_HANDLE=1
)
target_precompile_headers(${PROJECT_NAME} PRIVATE "../External/vulkan/vulkan.hpp")

target_include_directories(${PROJECT_NAME} PRIVATE
    .
    ../External/Vulkan
    ../External/rapidjson/include
    ../External/stb_image
    ../External/imgui
    ../External/type_safe/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	Core
    Vulkan::Vulkan
    imgui
    VulkanMemoryAllocator
)

if (WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE
        ../../Binaries/SDL2/Windows
        ../../Binaries/assimp/Windows
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ../../Binaries/SDL2/Include
        ../../Binaries/assimp/Include
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2
        assimp-vc142-mt
    )
endif()

set(OutputDir
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Binaries
)

if (WIN32)
    set(DLLS_TO_COPY
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Binaries/SDL2/Windows/SDL2.dll
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Binaries/assimp/Windows/assimp-vc142-mt.dll
    )
    foreach(DLL ${DLLS_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${DLL} "${OutputDir}"
            VERBATIM
        )
    endforeach()
endif()

# Copy resulting executable.
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${OutputDir}")