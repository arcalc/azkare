#pragma once
#include <QObject>
#include <QString>

#include "../core/AdhkarDatabase.h"

class AppSettings;

/**
 * Linux only: renders through ElKowar's Wacky Widgets (eww) using a private config
 * directory (~/.config/azkare-eww) so the user's own eww setup is never touched.
 */
class EwwBridge : public QObject
{
    Q_OBJECT
public:
    explicit EwwBridge(AppSettings &settings, QObject *parent = nullptr);

    static bool isAvailable();
    void initialize();   ///< write config, open the window (starts the eww daemon)
    void reconfigure();  ///< settings changed: rewrite config and reopen
    void showEntry(const AdhkarEntry &entry);
    void close();

private:
    void writeConfig();
    bool runEww(const QStringList &args);

    AppSettings &m_settings;
    QString m_configDir;
    bool m_open = false;
};
