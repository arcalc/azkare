#pragma once
#include <QObject>

#include "../core/AdhkarDatabase.h"

class AppSettings;
class QSystemTrayIcon;

/**
 * Sends a native desktop notification.
 *  Linux:   org.freedesktop.Notifications over D-Bus (replaces the previous one)
 *  Windows/macOS (and Linux without D-Bus): QSystemTrayIcon::showMessage, which Qt maps to
 *  the platform's native notifications.
 */
class NotificationDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit NotificationDispatcher(AppSettings &settings, QObject *parent = nullptr);

    void setTrayIcon(QSystemTrayIcon *tray) { m_tray = tray; }
    void send(const AdhkarEntry &entry);

private:
    bool sendViaDBus(const QString &title, const QString &body);

    AppSettings &m_settings;
    QSystemTrayIcon *m_tray = nullptr;
    quint32 m_lastId = 0;
};
