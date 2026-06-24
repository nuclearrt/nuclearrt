set(COPY_SOURCE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/copy/all")
if(PLATFORM_WEB)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/copy/web")
        list(APPEND COPY_SOURCE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/copy/web")
    endif()
elseif(PLATFORM_WINDOWS OR PLATFORM_MACOS OR PLATFORM_LINUX)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/copy/desktop")
        list(APPEND COPY_SOURCE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/copy/desktop")
    endif()
elseif(PLATFORM_IOS)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/copy/mobile")
        list(APPEND COPY_SOURCE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/copy/mobile")
    endif()
endif()

if (PLATFORM_MACOS)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/copy/mac")
        list(APPEND COPY_SOURCE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/copy/mac")
    endif()
endif()

if(PLATFORM_MACOS)
    set(COPY_DEST_DIR "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources")
elseif(PLATFORM_IOS)
    set(COPY_DEST_DIR "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/www")
elseif(MSVC)
    set(COPY_DEST_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
else()
    set(COPY_DEST_DIR "${CMAKE_BINARY_DIR}/bin")
endif()

set(copy_dirs_commands "")
foreach(source_dir IN LISTS COPY_SOURCE_DIRS)
    if(NOT PLATFORM_WEB AND NOT PLATFORM_IOS)
        list(APPEND copy_dirs_commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${source_dir}" "${COPY_DEST_DIR}")
    endif()
endforeach()

if(NOT PLATFORM_WEB AND NOT PLATFORM_IOS)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        ${copy_dirs_commands}
        COMMENT "Copying files from copy directory to build output"
    )
elseif(PLATFORM_WEB)
    foreach(source_dir IN LISTS COPY_SOURCE_DIRS)
        target_link_options(${PROJECT_NAME} PRIVATE "SHELL:--embed-file \"${source_dir}@/\"")
    endforeach()
elseif(PLATFORM_IOS)
    set(IOS_RESOURCES "")

    foreach(source_dir IN LISTS COPY_SOURCE_DIRS)
        file(GLOB_RECURSE DIR_FILES
            "${source_dir}/*"
        )
        list(APPEND IOS_RESOURCES ${DIR_FILES})
    endforeach()

    set_source_files_properties(${IOS_RESOURCES} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    target_sources(${PROJECT_NAME} PRIVATE ${IOS_RESOURCES})
endif()