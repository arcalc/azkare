#include "NotificationDispatcher.h"

#include <QDebug>

#if defined(Q_OS_LINUX)
#  include <QDBusInterface>
#  include <QDBusReply>
#  include <QVariantMap>
#endif

NotificationDispatcher::NotificationDispatcher(AppSettings    &settings,
                                               AdhkarDatabase &db,
                                               QObject        *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_db(db)
{}

void NotificationDispatcher::initialize()
{
    // macOS: request UNUserNotificationCenter authorization
#if defined(Q_OS_MACOS)
    extern void requestMacOSNotificationPermission();
    requestMacOSNotificationPermission();
#endif
    // Linux and Windows require no explicit permission request
}

void NotificationDispatcher::send(const AdhkarEntry &entry)
{
    if (!m_settings.notificationsEnabled()) return;

#if defined(Q_OS_LINUX)
    sendLinux(entry);
#elif defined(Q_OS_WIN)
    sendWindows(entry);
#elif defined(Q_OS_MACOS)
    sendMacOS(entry);
#endif
}

// ── Linux — D-Bus org.freedesktop.Notifications ───────────────────────────────

void NotificationDispatcher::sendLinux(const AdhkarEntry &entry)
{
#if defined(Q_OS_LINUX)
    QDBusInterface notif(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications"
    );

    if (!notif.isValid()) {
        emit notificationFailed("D-Bus notification interface not available.");
        return;
    }

    QString body = entry.textAr;
    if (m_settings.showSource() && !entry.sourceRef.isEmpty())
        body += "\n— " + entry.sourceRef;

    QVariantList args;
    args << "Azkare"             // app-name
         << quint32(0)           // replaces-id (0 = new)
         << "azkare"             // icon name (must be in icon theme or absolute path)
         << "أذكاري"             // summary
         << body                 // body
         << QStringList()        // actions
         << QVariantMap()        // hints
         << qint32(8000);        // timeout ms

    QDBusReply<quint32> reply = notif.callWithArgumentList(QDBus::AutoDetect, "Notify", args);
    if (!reply.isValid()) {
        emit notificationFailed(reply.error().message());
        return;
    }
    emit notificationSent();
#else
    Q_UNUSED(entry)
#endif
}

// ── Windows — Toast Notifications via WinRT (requires Windows 10+) ────────────

void NotificationDispatcher::sendWindows(const AdhkarEntry &entry)
{
#if defined(Q_OS_WIN)
    // WinRT Toast requires #include <winrt/...> and /std:c++17 or higher.
    // A minimal XML-based toast:
    QString body = entry.textAr;
    if (m_settings.showSource() && !entry.sourceRef.isEmpty())
        body += "\n— " + entry.sourceRef;

    // Convert to std::wstring for WinRT
    std::wstring title = L"\u0623\u0630\u0643\u0627\u0631\u064A"; // أذكاري
    std::wstring text  = body.toStdWString();

    // Simple WinRT wrapper — full implementation in platform/windows/WinToast.cpp
    extern void sendWindowsToast(const std::wstring &title, const std::wstring &body);
    sendWindowsToast(title, text);
    emit notificationSent();
#else
    Q_UNUSED(entry)
#endif
}

// ── macOS — UNUserNotificationCenter ─────────────────────────────────────────

void NotificationDispatcher::sendMacOS(const AdhkarEntry &entry)
{
#if defined(Q_OS_MACOS)
    QString body = entry.textAr;
    if (m_settings.showSource() && !entry.sourceRef.isEmpty())
        body += "\n— " + entry.sourceRef;

    // Bridged from Objective-C++ in platform/macos/MacNotifications.mm
    extern void sendMacOSNotification(const char *title, const char *body);
    sendMacOSNotification("أذكاري", body.toUtf8().constData());
    emit notificationSent();
#else
    Q_UNUSED(entry)
#endif
}
