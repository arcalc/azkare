set(AZKARE_SOURCES
    core/AdhkarDatabase.cpp
    core/AppSettings.cpp
    core/Ipc.cpp
    core/Localizer.cpp
    core/Scheduler.cpp
    core/Updater.cpp
    overlay/OverlayManager.cpp
    overlay/OverlayWidget.cpp
    overlay/EwwBridge.cpp
    notifications/NotificationDispatcher.cpp
    platform/AutostartManager.cpp
    platform/TrayController.cpp
    ui/SettingsWindow.cpp)
if(APPLE)
    list(APPEND AZKARE_SOURCES platform/macos/MacOverlay.mm)
endif()

add_library(azkare_lib STATIC ${AZKARE_SOURCES})
target_include_directories(azkare_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}/generated")
target_link_libraries(azkare_lib PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(azkare_lib PRIVATE /W4 /utf-8)
else()
    target_compile_options(azkare_lib PRIVATE -Wall -Wextra)
endif()

if(UNIX AND NOT APPLE)
    if(Qt6DBus_FOUND)
        target_link_libraries(azkare_lib PUBLIC Qt6::DBus)
        target_compile_definitions(azkare_lib PUBLIC AZKARE_HAS_DBUS)
    else()
        message(STATUS "Qt6 DBus not found: notifications fall back to the tray icon")
    endif()
    find_package(X11 QUIET)
    if(X11_FOUND)
        target_link_libraries(azkare_lib PUBLIC X11::X11)
        target_compile_definitions(azkare_lib PUBLIC AZKARE_HAS_X11)
    else()
        message(WARNING "libX11 headers not found: X11 'below all windows' hints disabled")
    endif()
    find_package(LayerShellQt CONFIG QUIET)
    if(LayerShellQt_FOUND)
        target_link_libraries(azkare_lib PUBLIC LayerShellQt::Interface)
        target_compile_definitions(azkare_lib PUBLIC AZKARE_HAS_LAYERSHELL)
        message(STATUS "LayerShellQt found: Wayland layer-shell overlay enabled")
    else()
        message(STATUS "LayerShellQt not found: no Wayland layer-shell (X11/XWayland only)")
    endif()
endif()
if(APPLE)
    target_link_libraries(azkare_lib PUBLIC "-framework AppKit" "-framework Foundation")
endif()
if(WIN32)
    target_link_libraries(azkare_lib PUBLIC advapi32 user32)
endif()
