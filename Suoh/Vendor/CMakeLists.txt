add_subdirectory(glm)
add_subdirectory(VulkanMemoryAllocator)
add_subdirectory(glfw)

set(ENABLE_GLSLANG_BINARIES  OFF  CACHE BOOL "")
set(ENABLE_HLSL              OFF  CACHE BOOL "")
set(ENABLE_CTEST             OFF  CACHE BOOL "")
set(ENABLE_OPT               OFF  CACHE BOOL "")
set(SKIP_GLSLANG_INSTALL     ON   CACHE BOOL "")
add_subdirectory(glslang)

add_subdirectory(meshoptimizer)

set(GLI_TEST_ENABLE OFF CACHE BOOL "")
add_subdirectory(gli)

