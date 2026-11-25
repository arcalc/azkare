#include "Ipc.h"

#include <QLocalSocket>
#include <QRegularExpression>
#include <QtGlobal>

static QString userTag()
{
    QString u = qEnvironmentVariable("USER");
    if (u.isEmpty()) u = qEnvironmentVariable("USERNAME");
    if (u.isEmpty()) u = QStringLiteral("user");
    u.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    return u;
}

QString Ipc::daemonName() { return QStringLiteral("azkare-ipc-") + userTag(); }
QString Ipc::settingsName() { return QStringLiteral("azkare-settings-") + userTag(); }

bool Ipc::send(const QString &name, const QString &command, QString *reply, int timeoutMs)
{
    QLocalSocket s;
    s.connectToServer(name);
    if (!s.waitForConnected(timeoutMs))
        return false;
    s.write((command + QLatin1Char('\n')).toUtf8());
    s.flush();
    if (s.state() == QLocalSocket::ConnectedState)
        s.waitForBytesWritten(timeoutMs);
    if (reply && s.waitForReadyRead(timeoutMs * 2))
        *reply = QString::fromUtf8(s.readAll()).trimmed();
    s.disconnectFromServer();
    return true;
}
