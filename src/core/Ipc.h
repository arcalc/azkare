#pragma once
#include <QString>

/** Tiny line-based local-socket protocol between the daemon, the settings window and the CLI. */
namespace Ipc {
QString daemonName();    ///< socket of the running overlay/tray daemon (per user)
QString settingsName();  ///< socket of an open settings window (per user)
/** Sends one command line; returns false if nobody is listening. Reply (if any) goes to *reply. */
bool send(const QString &name, const QString &command, QString *reply = nullptr, int timeoutMs = 500);
}
