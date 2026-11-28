#include "NotificationDispatcher.h"

#include "../core/AppSettings.h"

#include <QSystemTrayIcon>
#include <QDebug>

#if defined(AZKARE_HAS_DBUS)
#  include <QDBusInterface>
#  include <QDBusReply>
#  include <QVariantMap>
#endif

NotificationDispatcher::NotificationDispatcher(AppSettings &settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

void NotificationDispatcher::send(const AdhkarEntry &e)
{
    if (!m_settings.notificationsEnabled() || !e.isValid())
        return;
    const QString title = QString::fromUtf8("أذكاري");
    QString body = e.textAr;
    if (m_settings.showSource() && e.hasSource())
        body += QLatin1Char('\n') + e.sourceLabelAr() + QLatin1Char('\n') + e.sourceLabelEn();
    if (e.repeat > 1)
        body += QStringLiteral("\n× %1").arg(e.repeat);

    if (sendViaDBus(title, body))
        return;
    if (m_tray && QSystemTrayIcon::supportsMessages())
        m_tray->showMessage(title, body, QSystemTrayIcon::NoIcon, 10000);
    else
        qWarning() << "[Notifications] no notification backend available";
}

bool NotificationDispatcher::sendViaDBus(const QString &title, const QString &body)
{
#if defined(AZKARE_HAS_DBUS)
    QDBusInterface iface(QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("/org/freedesktop/Notifications"),
                         QStringLiteral("org.freedesktop.Notifications"));
    if (!iface.isValid())
        return false;
    iface.setTimeout(2000);
    QVariantMap hints;
    hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("azkare"));
    const QVariantList args{QStringLiteral("Azkare"), m_lastId, QStringLiteral("azkare"), title,
                            body.toHtmlEscaped(), QStringList(), hints, qint32(10000)};
    const QDBusReply<quint32> reply = iface.callWithArgumentList(QDBus::AutoDetect, QStringLiteral("Notify"), args);
    if (!reply.isValid()) {
        qWarning() << "[Notifications] D-Bus error:" << reply.error().message();
        return false;
    }
    m_lastId = reply.value();
    return true;
#else
    Q_UNUSED(title) Q_UNUSED(body)
    return false;
#endif
}
