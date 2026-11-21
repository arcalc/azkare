set(NOTIF_SOURCES
    NotificationDispatcher.cpp
)

# macOS: Objective-C++ bridge
if(AZKARE_MACOS)
    list(APPEND NOTIF_SOURCES
        ../platform/macos/MacNotifications.mm
    )
    set_source_files_properties(
        ../platform/macos/MacNotifications.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()

add_library(azkare_notifications STATIC ${NOTIF_SOURCES})

target_include_directories(azkare_notifications PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(azkare_notifications
    PUBLIC
        Qt6::Core
        azkare_core
)

if(AZKARE_LINUX)
    target_link_libraries(azkare_notifications PUBLIC Qt6::DBus)
endif()

if(AZKARE_MACOS)
    target_link_libraries(azkare_notifications PUBLIC
        "-framework UserNotifications"
        "-framework Foundation"
    )
endif()
