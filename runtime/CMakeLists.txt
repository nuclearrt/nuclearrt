cmake_minimum_required(VERSION 3.14)
project(nuclearrt-runtime CXX)
set(FRIENDLY_NAME "NuclearRT-Runtime")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS" FORCE)
endif()

# Platform-specific compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4102")
elseif(NOT EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused-label")
endif()

# Add cmake modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if (EMSCRIPTEN)
  add_compile_options(-pthread)
  add_link_options(-sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=2gb -sMAXIMUM_MEMORY=2gb -sOFFSCREENCANVAS_SUPPORT=1 -sPTHREAD_POOL_SIZE_STRICT=0 -sFORCE_FILESYSTEM=1 -sWASMFS=1 -sEXIT_RUNTIME=1 -sUSE_HARFBUZZ=1)
endif()

include(FetchContent)
function(include_dependency libName gitURL gitTag)
    # setup the declare
    FetchContent_Declare(${libName}
            GIT_REPOSITORY ${gitURL}
            GIT_TAG        ${gitTag}
            GIT_SHALLOW    FALSE
            GIT_PROGRESS   TRUE
    )

    FetchContent_MakeAvailable(${libName})
endfunction()

find_package(SDL3 QUIET)
if (NOT SDL3_FOUND)
    message(STATUS "Getting SDL3 from Github")
    include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.4.0)
else()
    message(STATUS "Using local SDL3")
endif()

set(SDLIMAGE_VENDORED ON)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_TIF OFF)

find_package(SDL3_image QUIET)
if (NOT SDL3_image_FOUND)
    message(STATUS "Getting SDL3_image from Github")
    include_dependency(SDL3_image https://github.com/libsdl-org/SDL_image.git release-3.2.6)
else()
    message(STATUS "Using local SDL3_image")
endif()

set(SDLTTF_VENDORED ON)

find_package(SDL3_ttf QUIET)
if (NOT SDL3_ttf_FOUND)
    message(STATUS "Getting SDL3_ttf from Github")
    include_dependency(SDL3_ttf https://github.com/libsdl-org/SDL_ttf.git release-3.2.2)
else()
    message(STATUS "Using local SDL3_ttf")
endif()

if(WIN32 OR (UNIX AND NOT APPLE AND NOT EMSCRIPTEN))
    find_package(GLEW QUIET)
    if (NOT GLEW_FOUND)
        message(STATUS "Getting GLEW from Github")
        set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
        set(ONLY_LIBS ON CACHE BOOL "" FORCE)
        include_dependency(glew https://github.com/Perlmint/glew-cmake.git glew-cmake-2.2.0-1)
    else()
        message(STATUS "Using local GLEW")
    endif()
endif()

find_package(OpenGL REQUIRED)

add_compile_definitions(NUCLEAR_BACKEND_SDL3)

if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    add_compile_definitions(PLATFORM_LINUX)
elseif(EMSCRIPTEN)
    add_compile_definitions(PLATFORM_WEB)
endif()

# Configure debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

# Dear ImGui setup
if(NOT EMSCRIPTEN)
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
    
    # Include directories for ImGui
    include_directories(
        ${IMGUI_PATH}
        ${IMGUI_PATH}/backends
    )
else()
    set(IMGUI_SOURCES "")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Automatically include all source and header files
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

# Create executable
if(APPLE AND NOT EMSCRIPTEN)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        PRODUCT_NAME "${FRIENDLY_NAME}"
        MACOSX_BUNDLE_EXECUTABLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "${FRIENDLY_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.nuclearrt.${PROJECT_NAME}"
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})
endif()

# Platform-specific linking
if(NOT EMSCRIPTEN)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_image_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_ttf_INCLUDE_DIRS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::Headers
)

# Set output directory
if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web"
    SUFFIX ".html"
)
elseif(APPLE AND NOT EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# Copy directory setup
set(COPY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/copy")
set(ASSETS_SOURCE_FILE "${COPY_SOURCE_DIR}/assets.pak")

# Set output destination based on platform
if(APPLE AND NOT EMSCRIPTEN)
    set(COPY_DEST_DIR "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources")
elseif(MSVC)
    set(COPY_DEST_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
else()
    set(COPY_DEST_DIR "${CMAKE_BINARY_DIR}/bin")
endif()

if(NOT EMSCRIPTEN)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${COPY_SOURCE_DIR} ${COPY_DEST_DIR}
        COMMENT "Copying files from copy directory to build output"
    )
else()
    target_link_options(${PROJECT_NAME} PRIVATE --embed-file ${ASSETS_SOURCE_FILE}@assets.pak)
endif()

#copy SDL3 dlls
if(WIN32 AND NOT EMSCRIPTEN)
    message(STATUS "Copying SDL3 dlls to output directory")
    foreach(sdl_target IN ITEMS SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)
        if(TARGET ${sdl_target})
            add_custom_command(
                TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${sdl_target}>
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
endif()

# Install target
if(APPLE AND NOT EMSCRIPTEN)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
endif()

# Additional platform-specific linking
if(WIN32 AND NOT EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm OpenGL::GL libglew_static)
elseif(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 OpenGL::GL libglew_static m pthread)
elseif(APPLE AND NOT EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
endif()