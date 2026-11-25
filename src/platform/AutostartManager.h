#pragma once
#include <QObject>
#include <QString>

/**
 * Login autostart.
 *  Linux:   ~/.config/autostart/azkare.desktop
 *  Windows: HKCU\Software\Microsoft\Windows\CurrentVersion\Run
 *  macOS:   ~/Library/LaunchAgents/io.github.arcalc.azkare.plist
 */
class AutostartManager : public QObject
{
    Q_OBJECT
public:
    explicit AutostartManager(QObject *parent = nullptr) : QObject(parent) {}
    bool isEnabled() const;
    bool setEnabled(bool on);
    static QString entryPath();   ///< file that is created (empty on Windows)
};
