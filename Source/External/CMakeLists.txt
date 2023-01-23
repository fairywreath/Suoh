add_subdirectory(glm)
add_subdirectory(VulkanMemoryAllocator)

add_library(imgui
    imgui/imconfig.h
    imgui/imgui_draw.cpp
    imgui/imgui_impl_sdl.cpp
    imgui/imgui_impl_sdl.h
    imgui/imgui_internal.h
    imgui/imgui_memory_editor.h
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
    imgui/stb_image.h
    imgui/TextEditor.cpp
    imgui/TextEditor.h
)
target_include_directories(imgui PRIVATE
    ../../Binaries/SDL2/include
)