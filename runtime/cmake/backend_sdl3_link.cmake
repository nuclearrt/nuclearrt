#copy SDL3 dlls
if(PLATFORM_WINDOWS)
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

if(NOT PLATFORM_WEB)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_image_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_ttf_INCLUDE_DIRS})
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC 
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
    SDL3::Headers
)

if(PLATFORM_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm OpenGL::GL libglew_static)
elseif(PLATFORM_LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 OpenGL::GL libglew_static m pthread)
elseif(PLATFORM_MACOS)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
elseif (PLATFORM_IOS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGLES_FRAMEWORK})
endif()