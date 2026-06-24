if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

if(NOT PLATFORM_WEB)
    set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui")
    set(IMGUI_SOURCES
        ${IMGUI_PATH}/imgui.cpp
        ${IMGUI_PATH}/imgui_demo.cpp
        ${IMGUI_PATH}/imgui_draw.cpp
        ${IMGUI_PATH}/imgui_tables.cpp
        ${IMGUI_PATH}/imgui_widgets.cpp
        ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
    )
    
    include_directories(
        ${IMGUI_PATH}
        ${IMGUI_PATH}/backends
    )
else()
    set(IMGUI_SOURCES "")
endif()
