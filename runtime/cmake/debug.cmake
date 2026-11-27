option(NUCLEAR_DEBUG_UI "Enable the in-application Debug UI toggle (always enabled in Debug)" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

if(NOT PLATFORM_WEB)
    add_compile_definitions("$<$<OR:$<BOOL:${NUCLEAR_DEBUG_UI}>,$<CONFIG:Debug>>:NUCLEAR_DEBUG_UI>")

    set(IMGUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui")
    set(IMGUI_SOURCES
        ${IMGUI_PATH}/imgui.cpp
        ${IMGUI_PATH}/imgui_demo.cpp
        ${IMGUI_PATH}/imgui_draw.cpp
        ${IMGUI_PATH}/imgui_tables.cpp
        ${IMGUI_PATH}/imgui_widgets.cpp
    )

    if(PLATFORM_BACKEND STREQUAL "SDL3")
        list(APPEND IMGUI_SOURCES
            ${IMGUI_PATH}/backends/imgui_impl_sdl3.cpp
            ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
        )
    elseif(PLATFORM_BACKEND STREQUAL "SDL2")
        list(APPEND IMGUI_SOURCES
            ${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp
            ${IMGUI_PATH}/backends/imgui_impl_sdlrenderer2.cpp
        )
    endif()
    
    include_directories(
        ${IMGUI_PATH}
        ${IMGUI_PATH}/backends
    )
else()
    set(IMGUI_SOURCES "")
endif()
