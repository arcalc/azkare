#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include "../core/AdhkarDatabase.h"
#include "../core/AppSettings.h"

/**
 * Linux-only: drives ElKowar's Wacky Widgets (eww) as an alternative
 * rendering backend.
 *
 * On initialize():
 *   1. Writes ~/.config/eww/azkare/eww.yuck  (widget definition)
 *   2. Writes ~/.config/eww/azkare/eww.scss  (styling)
 *   3. Launches eww daemon and opens the azkare window.
 *
 * On showEntry(): updates the eww variable `azkare-text` via eww update.
 */
class EwwBridge : public QObject
{
    Q_OBJECT

public:
    explicit EwwBridge(AppSettings &settings, QObject *parent = nullptr);
    ~EwwBridge() override;

    /** Returns true if eww binary is found on PATH. */
    static bool isAvailable();

    void initialize();
    void showEntry(const AdhkarEntry &entry);
    void close();

private:
    void writeEwwConfig();
    void startDaemon();
    void runEww(const QStringList &args);

    AppSettings &m_settings;
    QString      m_configDir;
    bool         m_running = false;
};
