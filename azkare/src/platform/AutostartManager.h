#pragma once

#include <QObject>
#include <QString>

/**
 * Manages registering/unregistering Azkare as a login-time autostart entry.
 *
 *  Linux   — XDG autostart: ~/.config/autostart/azkare.desktop
 *  Windows — HKCU\Software\Microsoft\Windows\CurrentVersion\Run
 *  macOS   — ~/Library/LaunchAgents/io.github.arcalc.azkare.plist
 */
class AutostartManager : public QObject
{
    Q_OBJECT

public:
    explicit AutostartManager(QObject *parent = nullptr);

    bool isEnabled() const;
    bool setEnabled(bool on);   ///< Returns true on success

private:
    bool enableLinux();
    bool disableLinux();
    bool enableWindows();
    bool disableWindows();
    bool enableMacOS();
    bool disableMacOS();

    QString executablePath() const;
};
