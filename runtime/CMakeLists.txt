cmake_minimum_required(VERSION 3.14)
project(nuclearrt-runtime C CXX)
set(FRIENDLY_NAME "NuclearRT-Runtime")
set(DOMAIN_NAME "com.nuclearrt.runtime")
set(APP_VERSION "1.0.0")
set(APP_AUTHOR "NuclearRT")
set(APP_COPYRIGHT "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(BACKEND_OPTIONS "SDL3" "SDL2")
set(DEFAULT_PLATFORM_BACKEND "SDL3")

#switch only supports SDL2
if(NINTENDO_SWITCH)
    set(DEFAULT_PLATFORM_BACKEND "SDL2")
endif()

set(PLATFORM_BACKEND "${DEFAULT_PLATFORM_BACKEND}" CACHE STRING "Select the platform backend")

set_property(CACHE PLATFORM_BACKEND PROPERTY STRINGS ${BACKEND_OPTIONS})

if(NOT PLATFORM_BACKEND IN_LIST BACKEND_OPTIONS)
    message(FATAL_ERROR "Invalid backend '${PLATFORM_BACKEND}'. Choose from: ${BACKEND_OPTIONS}")
endif()

include(cmake/platform_setup.cmake)

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

if(PLATFORM_BACKEND STREQUAL "SDL3")
    include(cmake/backend_sdl3_setup.cmake)
elseif(PLATFORM_BACKEND STREQUAL "SDL2")
    include(cmake/backend_sdl2_setup.cmake)
endif()

include(cmake/debug.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

# Create executable
if(PLATFORM_MACOS OR PLATFORM_IOS)

    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PRODUCT_NAME "${FRIENDLY_NAME}"
        MACOSX_BUNDLE_EXECUTABLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${FRIENDLY_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "${APP_COPYRIGHT}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${DOMAIN_NAME}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${DOMAIN_NAME}"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "YES"
    )
elseif(PLATFORM_WINDOWS)
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES} resources.rc)
    # Always recompile the resource script
    set_source_files_properties(resources.rc PROPERTIES VS_SETTINGS "MinimalRebuildFromTracking=false")
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})
endif()

# Set output directory
if(PLATFORM_WEB)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "index"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
        SUFFIX ".html"
    )
    target_link_options(${PROJECT_NAME} PRIVATE "SHELL:--shell-file \"${CMAKE_CURRENT_SOURCE_DIR}/web_shell.html\"")
elseif(PLATFORM_MACOS OR PLATFORM_IOS)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

include(cmake/assets.cmake)

if(PLATFORM_BACKEND STREQUAL "SDL3")
    include(cmake/backend_sdl3_link.cmake)
elseif(PLATFORM_BACKEND STREQUAL "SDL2")
    include(cmake/backend_sdl2_link.cmake)
endif()

include(cmake/lz4.cmake)

if(PLATFORM_SWITCH)
    nx_generate_nacp(
        OUTPUT ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.nacp
        NAME "${FRIENDLY_NAME}"
        AUTHOR "${APP_AUTHOR}"
        VERSION "${APP_VERSION}"
    )

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/romfs)

    set(SWITCH_ICON_ARGS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icons/switch.jpg")
        set(SWITCH_ICON_ARGS ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/switch.jpg")
    endif()

    nx_create_nro(${PROJECT_NAME}
        OUTPUT ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.nro
        NACP ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.nacp
        ROMFS ${CMAKE_BINARY_DIR}/romfs
        ${SWITCH_ICON_ARGS}
    )
endif()

# Install target
if(PLATFORM_MACOS)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
endif()