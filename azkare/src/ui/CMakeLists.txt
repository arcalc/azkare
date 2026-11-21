add_library(azkare_ui STATIC
    SettingsWindow.cpp
)

target_include_directories(azkare_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(azkare_ui
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        azkare_core
        azkare_overlay
        azkare_notifications
        azkare_platform
)
