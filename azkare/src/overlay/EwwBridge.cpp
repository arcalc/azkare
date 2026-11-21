#include "EwwBridge.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>

EwwBridge::EwwBridge(AppSettings &settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_configDir = QDir::homePath() + "/.config/eww/azkare";
}

EwwBridge::~EwwBridge() { close(); }

bool EwwBridge::isAvailable()
{
    return !QStandardPaths::findExecutable("eww").isEmpty();
}

void EwwBridge::initialize()
{
    QDir().mkpath(m_configDir);
    writeEwwConfig();
    startDaemon();
}

void EwwBridge::showEntry(const AdhkarEntry &entry)
{
    if (!m_running) return;

    // Escape double quotes in Arabic text
    QString text = entry.textAr;
    text.replace('"', "'");

    QString source = m_settings.showSource() ? entry.sourceRef : QString();

    runEww({"update", QString("azkare-text=\"%1\"").arg(text)});
    runEww({"update", QString("azkare-source=\"%1\"").arg(source)});
}

void EwwBridge::close()
{
    if (m_running) {
        runEww({"close", "azkare-window"});
        m_running = false;
    }
}

// ── Private ──────────────────────────────────────────────────────────────────

void EwwBridge::writeEwwConfig()
{
    // ── eww.yuck ──────────────────────────────────────────────────────────
    QString yuck = R"yuck(
(defvar azkare-text "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ")
(defvar azkare-source "")

(defwidget azkare-widget []
  (box :orientation "v"
       :class "azkare-box"
    (label :text azkare-text
           :class "azkare-text"
           :wrap true
           :justify "right")
    (label :text azkare-source
           :class "azkare-source"
           :visible {azkare-source != ""})))

(defwindow azkare-window
  :monitor 0
  :geometry (geometry
              :x "%POSX%"
              :y "%POSY%"
              :width "50%"
              :anchor "%ANCHOR%")
  :stacking "bottom"
  :exclusive false
  (azkare-widget))
)yuck";

    // Substitute position settings
    QString posX    = "0%";
    QString posY    = "90%";
    QString anchor  = "bottom-center";

    switch (m_settings.overlayPosition()) {
    case OverlayPosition::TopLeft:    posX="2%"; posY="2%";  anchor="top-left";     break;
    case OverlayPosition::TopCenter:  posX="0%"; posY="2%";  anchor="top-center";   break;
    case OverlayPosition::TopRight:   posX="2%"; posY="2%";  anchor="top-right";    break;
    case OverlayPosition::Center:     posX="0%"; posY="0%";  anchor="center";       break;
    case OverlayPosition::BottomLeft: posX="2%"; posY="90%"; anchor="bottom-left";  break;
    case OverlayPosition::BottomCenter: posX="0%"; posY="90%"; anchor="bottom-center"; break;
    case OverlayPosition::BottomRight: posX="2%"; posY="90%"; anchor="bottom-right"; break;
    default: break;
    }

    yuck.replace("%POSX%",  posX);
    yuck.replace("%POSY%",  posY);
    yuck.replace("%ANCHOR%", anchor);

    QFile yuckFile(m_configDir + "/eww.yuck");
    if (yuckFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        QTextStream(&yuckFile) << yuck;

    // ── eww.scss ──────────────────────────────────────────────────────────
    QColor color = m_settings.textColor();
    QString scss = QString(R"scss(
.azkare-box {
  background: transparent;
  padding: 12px;
}
.azkare-text {
  font-family: "%1";
  font-size: %2px;
  color: %3;
  text-shadow: 2px 2px 4px rgba(0,0,0,0.8);
  direction: rtl;
}
.azkare-source {
  font-family: "%1";
  font-size: %4px;
  color: rgba(%5,%6,%7,0.65);
  direction: rtl;
  margin-top: 4px;
}
)scss")
        .arg(m_settings.fontFamily())
        .arg(m_settings.fontSize())
        .arg(color.name())
        .arg(m_settings.fontSize() / 2)
        .arg(color.red()).arg(color.green()).arg(color.blue());

    QFile scssFile(m_configDir + "/eww.scss");
    if (scssFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        QTextStream(&scssFile) << scss;

    qInfo() << "[EwwBridge] Config written to" << m_configDir;
}

void EwwBridge::startDaemon()
{
    // Kill any existing azkare eww daemon
    QProcess::execute("eww", {"--config", m_configDir, "kill"});

    // Start daemon
    QProcess *daemon = new QProcess(this);
    daemon->start("eww", {"--config", m_configDir, "daemon"});
    daemon->waitForStarted(2000);

    // Open our window
    runEww({"--config", m_configDir, "open", "azkare-window"});
    m_running = true;
    qInfo() << "[EwwBridge] eww daemon started and window opened.";
}

void EwwBridge::runEww(const QStringList &args)
{
    QStringList fullArgs = {"--config", m_configDir};
    fullArgs << args;
    QProcess::execute("eww", fullArgs);
}
