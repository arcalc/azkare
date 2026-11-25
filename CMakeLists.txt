cmake_minimum_required(VERSION 3.20)

# VERSION is the single source of truth (see scripts/release.sh)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" AZKARE_VERSION_RAW)
string(STRIP "${AZKARE_VERSION_RAW}" AZKARE_VERSION_STR)
if(NOT AZKARE_VERSION_STR MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must look like X.Y.Z, got '${AZKARE_VERSION_STR}'")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

project(Azkare VERSION ${AZKARE_VERSION_STR} DESCRIPTION "Adhkar & Duas on your desktop"
        HOMEPAGE_URL "https://github.com/arcalc/azkare" LANGUAGES CXX)
if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

option(AZKARE_FETCH_FONTS "Download the bundled open-source Arabic fonts at configure time if missing" ON)

# ── Git hash + build date -> generated Version.h ─────────────────────────────
set(AZKARE_GIT_HASH "unknown")
find_package(Git QUIET)
if(Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _hash OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE _rc)
    if(_rc EQUAL 0 AND _hash)
        set(AZKARE_GIT_HASH "${_hash}")
    endif()
endif()
string(TIMESTAMP AZKARE_BUILD_DATE "%Y-%m-%d")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h" @ONLY)

# ── Dependencies ─────────────────────────────────────────────────────────────
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Network OPTIONAL_COMPONENTS DBus)

# ── Application ──────────────────────────────────────────────────────────────
add_subdirectory(src)

qt_add_executable(azkare src/main.cpp)
target_include_directories(azkare PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(azkare PRIVATE azkare_lib)

# ── Embedded resources: dataset, translations, fonts ─────────────────────────
set(AZKARE_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
file(GLOB AZKARE_FONTS CONFIGURE_DEPENDS "${AZKARE_RES_DIR}/fonts/*.ttf" "${AZKARE_RES_DIR}/fonts/*.otf")
if(NOT AZKARE_FONTS AND AZKARE_FETCH_FONTS AND NOT WIN32)
    message(STATUS "No fonts in resources/fonts - fetching them (scripts/fetch-fonts.sh)")
    execute_process(COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch-fonts.sh" RESULT_VARIABLE _frc OUTPUT_QUIET)
    file(GLOB AZKARE_FONTS "${AZKARE_RES_DIR}/fonts/*.ttf" "${AZKARE_RES_DIR}/fonts/*.otf")
endif()
if(NOT AZKARE_FONTS)
    message(WARNING "No Arabic fonts bundled - the app will use system fonts. Run scripts/fetch-fonts.sh and re-run cmake.")
endif()
qt_add_resources(azkare azkare_resources PREFIX "/" BASE "${AZKARE_RES_DIR}"
    FILES "${AZKARE_RES_DIR}/data/adhkar.json"
          "${AZKARE_RES_DIR}/i18n/ar.json"
          "${AZKARE_RES_DIR}/i18n/en.json"
          ${AZKARE_FONTS})

# ── Install ──────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS azkare RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/azkare.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/azkare.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
