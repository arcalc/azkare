cmake_minimum_required(VERSION 3.20)

project(Azkare
    VERSION 1.0.0
    DESCRIPTION "Cross-platform desktop Adhkar & Dua engine"
    HOMEPAGE_URL "https://github.com/arcalc/azkare"
    LANGUAGES CXX
)

# ── C++ Standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── CMake modules ─────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ── Platform detection ───────────────────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(AZKARE_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(AZKARE_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(AZKARE_MACOS TRUE)
endif()

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    DBus   # Linux D-Bus notifications
)
qt_standard_project_setup()

# ── nlohmann/json (header-only) ───────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# ── SQLite3 ───────────────────────────────────────────────────────────────────
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    # Fallback: build bundled sqlite amalgamation
    add_library(sqlite3 STATIC third_party/sqlite/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC third_party/sqlite)
    set(SQLite3_LIBRARIES sqlite3)
    set(SQLite3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/sqlite")
endif()

# ── libcurl (optional — for update checker) ───────────────────────────────────
find_package(CURL QUIET)
if(CURL_FOUND)
    add_compile_definitions(AZKARE_HAS_CURL)
endif()

# ── Linux: Wayland & X11 ─────────────────────────────────────────────────────
if(AZKARE_LINUX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(WAYLAND_CLIENT QUIET wayland-client)
    pkg_check_modules(WLR_LAYER_SHELL QUIET wlr-layer-shell-unstable-v1)
    pkg_check_modules(GTK_LAYER_SHELL QUIET gtk-layer-shell-0)
    pkg_check_modules(X11 QUIET x11)

    if(WAYLAND_CLIENT_FOUND)
        add_compile_definitions(AZKARE_HAS_WAYLAND)
    endif()
    if(X11_FOUND)
        add_compile_definitions(AZKARE_HAS_X11)
    endif()
endif()

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Subdirectories ────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Resources: embed data files ───────────────────────────────────────────────
qt_add_resources(AZKARE_RESOURCES
    PREFIX "/"
    FILES
        resources/data/adhkar.json
        resources/i18n/ar.json
        resources/i18n/en.json
        resources/fonts/fonts_manifest.json
)

# ── Main executable ───────────────────────────────────────────────────────────
qt_add_executable(azkare
    src/main.cpp
    ${AZKARE_RESOURCES}
)

target_link_libraries(azkare
    PRIVATE
        azkare_core
        azkare_overlay
        azkare_notifications
        azkare_platform
        azkare_ui
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        nlohmann_json::nlohmann_json
        ${SQLite3_LIBRARIES}
)

if(AZKARE_LINUX)
    target_link_libraries(azkare PRIVATE Qt6::DBus)
    if(CURL_FOUND)
        target_link_libraries(azkare PRIVATE CURL::libcurl)
    endif()
    if(WAYLAND_CLIENT_FOUND)
        target_link_libraries(azkare PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    endif()
    if(X11_FOUND)
        target_link_libraries(azkare PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# ── Install rules ─────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS azkare
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(AZKARE_LINUX)
    install(FILES packaging/linux/azkare.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES packaging/linux/azkare.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
endif()

# ── CPack packaging ───────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "azkare")
set(CPACK_PACKAGE_VENDOR "arcalc")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CMAKE_PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${CMAKE_HOMEPAGE_URL}")

if(AZKARE_LINUX)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mahmoud El Sheikh <arcalc@proton.me>")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6, libqt6network6")
elseif(AZKARE_WINDOWS)
    set(CPACK_GENERATOR "NSIS")
elseif(AZKARE_MACOS)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
