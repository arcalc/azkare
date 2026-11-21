#pragma once

#include <QObject>
#include "../core/AdhkarDatabase.h"
#include "../core/AppSettings.h"

/**
 * Sends a native desktop notification for a given Adhkar entry.
 *
 *  Linux   — libnotify via D-Bus (org.freedesktop.Notifications)
 *  Windows — Windows Toast Notifications via WinRT
 *  macOS   — UNUserNotificationCenter (bridged via Objective-C++)
 */
class NotificationDispatcher : public QObject
{
    Q_OBJECT

public:
    explicit NotificationDispatcher(AppSettings    &settings,
                                    AdhkarDatabase &db,
                                    QObject        *parent = nullptr);

    /** Request any permissions needed (macOS, Windows). */
    void initialize();

    /** Send a notification for the given entry. */
    void send(const AdhkarEntry &entry);

signals:
    void notificationSent();
    void notificationFailed(const QString &reason);

private:
    void sendLinux(const AdhkarEntry &entry);
    void sendWindows(const AdhkarEntry &entry);
    void sendMacOS(const AdhkarEntry &entry);

    AppSettings    &m_settings;
    AdhkarDatabase &m_db;
};
