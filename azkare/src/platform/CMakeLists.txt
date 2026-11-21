set(PLATFORM_SOURCES
    AutostartManager.cpp
    TrayController.cpp
)

if(AZKARE_MACOS)
    list(APPEND PLATFORM_SOURCES
        macos/MacOverlay.mm
    )
    set_source_files_properties(
        macos/MacOverlay.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

add_library(azkare_platform STATIC ${PLATFORM_SOURCES})

target_include_directories(azkare_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(azkare_platform
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        azkare_core
)

if(AZKARE_WINDOWS)
    target_link_libraries(azkare_platform PUBLIC advapi32)
endif()

if(AZKARE_MACOS)
    target_link_libraries(azkare_platform PUBLIC
        "-framework AppKit"
        "-framework Foundation"
    )
endif()
