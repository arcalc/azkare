cmake_minimum_required(VERSION 3.20)

# ── Read VERSION file (single source of truth) ────────────────────────────────
file(READ "${CMAKE_SOURCE_DIR}/VERSION" AZKARE_VERSION_RAW)
string(STRIP "${AZKARE_VERSION_RAW}" AZKARE_VERSION_STR)

project(Azkare
    VERSION "${AZKARE_VERSION_STR}"
    DESCRIPTION "Cross-platform desktop Adhkar & Dua engine"
    HOMEPAGE_URL "https://github.com/arcalc/azkare"
    LANGUAGES CXX
)

# ── C++ Standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ── Git hash & build date for Version.h ──────────────────────────────────────
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE AZKARE_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT AZKARE_GIT_HASH)
    set(AZKARE_GIT_HASH "unknown")
endif()

string(TIMESTAMP AZKARE_BUILD_DATE "%Y-%m-%d")

# Auto-generate src/core/Version.h
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/Version.h.in"
    "${CMAKE_BINARY_DIR}/generated/Version.h"
    @ONLY
)

# ── Platform detection ────────────────────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(AZKARE_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(AZKARE_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(AZKARE_MACOS TRUE)
endif()

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
if(AZKARE_LINUX)
    find_package(Qt6 QUIET OPTIONAL_COMPONENTS DBus)
endif()
qt_standard_project_setup()

# ── nlohmann/json ─────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# ── SQLite3 ───────────────────────────────────────────────────────────────────
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    add_library(sqlite3 STATIC third_party/sqlite/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC third_party/sqlite)
    set(SQLite3_LIBRARIES sqlite3)
    set(SQLite3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/third_party/sqlite")
endif()

# ── Linux optional deps ───────────────────────────────────────────────────────
if(AZKARE_LINUX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(WAYLAND_CLIENT QUIET wayland-client)
        pkg_check_modules(X11 QUIET x11)
    endif()
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

# ── Qt resources ─────────────────────────────────────────────────────────────
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

target_include_directories(azkare PRIVATE
    "${CMAKE_BINARY_DIR}/generated"   # for Version.h
)

target_link_libraries(azkare
    PRIVATE
        azkare_core
        azkare_overlay
        azkare_notifications
        azkare_platform
        azkare_ui
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network
        nlohmann_json::nlohmann_json
        ${SQLite3_LIBRARIES}
)

if(AZKARE_LINUX)
    if(Qt6DBus_FOUND)
        target_link_libraries(azkare PRIVATE Qt6::DBus)
    endif()
    if(X11_FOUND)
        target_link_libraries(azkare PRIVATE ${X11_LIBRARIES})
    endif()
    if(WAYLAND_CLIENT_FOUND)
        target_link_libraries(azkare PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    endif()
endif()

if(AZKARE_MACOS)
    target_link_libraries(azkare PRIVATE "-framework AppKit" "-framework Foundation")
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS azkare RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(AZKARE_LINUX)
    install(FILES packaging/linux/azkare.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

# ── CPack ─────────────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "azkare")
set(CPACK_PACKAGE_VENDOR "arcalc")
set(CPACK_PACKAGE_VERSION "${AZKARE_VERSION_STR}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${CMAKE_HOMEPAGE_URL}")
if(AZKARE_LINUX)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mahmoud El Sheikh <arcalc@proton.me>")
elseif(AZKARE_WINDOWS)
    set(CPACK_GENERATOR "NSIS")
elseif(AZKARE_MACOS)
    set(CPACK_GENERATOR "DragNDrop")
endif()
include(CPack)
