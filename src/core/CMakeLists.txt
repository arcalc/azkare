add_library(azkare_core STATIC
    AdhkarDatabase.cpp
    AppSettings.cpp
    Scheduler.cpp
    Updater.cpp
)

target_include_directories(azkare_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(azkare_core
    PUBLIC
        Qt6::Core
        Qt6::Network
        nlohmann_json::nlohmann_json
)
