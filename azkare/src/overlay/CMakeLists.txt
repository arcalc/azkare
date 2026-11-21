set(OVERLAY_SOURCES
    OverlayManager.cpp
    OverlayWidget.cpp
    EwwBridge.cpp
)

add_library(azkare_overlay STATIC ${OVERLAY_SOURCES})

target_include_directories(azkare_overlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(azkare_overlay
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        azkare_core
)

if(AZKARE_LINUX AND X11_FOUND)
    target_link_libraries(azkare_overlay PUBLIC ${X11_LIBRARIES})
    target_include_directories(azkare_overlay PRIVATE ${X11_INCLUDE_DIR})
endif()
