#include "EwwBridge.h"

#include "../core/AppSettings.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>

EwwBridge::EwwBridge(AppSettings &settings, QObject *parent)
    : QObject(parent), m_settings(settings),
      m_configDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/azkare-eww"))
{}

bool EwwBridge::isAvailable() { return !QStandardPaths::findExecutable(QStringLiteral("eww")).isEmpty(); }

bool EwwBridge::runEww(const QStringList &args)
{
    QStringList full{QStringLiteral("--config"), m_configDir};
    full << args;
    return QProcess::execute(QStringLiteral("eww"), full) == 0;
}

void EwwBridge::initialize()
{
    QDir().mkpath(m_configDir);
    writeConfig();
    runEww({QStringLiteral("close"), QStringLiteral("azkare-window")});
    m_open = runEww({QStringLiteral("open"), QStringLiteral("azkare-window")});
    if (!m_open)
        qWarning() << "[EwwBridge] 'eww open' failed - check `eww --config" << m_configDir << "logs`";
}

void EwwBridge::reconfigure()
{
    runEww({QStringLiteral("kill")});
    initialize();
}

void EwwBridge::showEntry(const AdhkarEntry &e)
{
    if (!m_open)
        return;
    QStringList meta;
    if (m_settings.showSource() && e.hasSource()) meta << e.sourceLabelAr() << e.sourceLabelEn();
    if (e.repeat > 1) meta << QStringLiteral("× %1").arg(e.repeat);
    // No shell is involved, so the values are passed verbatim (no quoting needed).
    runEww({QStringLiteral("update"),
            QStringLiteral("azkare-text=") + e.textAr,
            QStringLiteral("azkare-translation=") + (m_settings.showTranslation() ? e.translationEn : QString()),
            QStringLiteral("azkare-meta=") + meta.join(QLatin1Char('\n'))});
}

void EwwBridge::close()
{
    if (m_open) {
        runEww({QStringLiteral("kill")});
        m_open = false;
    }
}

void EwwBridge::writeConfig()
{
    QString anchor = QStringLiteral("bottom center");
    QString x = QStringLiteral("0px"), y = QStringLiteral("24px");
    switch (m_settings.overlayPosition()) {
    case OverlayPosition::TopLeft:      anchor = QStringLiteral("top left"); x = QStringLiteral("24px"); break;
    case OverlayPosition::TopCenter:    anchor = QStringLiteral("top center"); break;
    case OverlayPosition::TopRight:     anchor = QStringLiteral("top right"); x = QStringLiteral("24px"); break;
    case OverlayPosition::Center:       anchor = QStringLiteral("center"); y = QStringLiteral("0px"); break;
    case OverlayPosition::BottomLeft:   anchor = QStringLiteral("bottom left"); x = QStringLiteral("24px"); break;
    case OverlayPosition::BottomCenter: break;
    case OverlayPosition::BottomRight:  anchor = QStringLiteral("bottom right"); x = QStringLiteral("24px"); break;
    case OverlayPosition::Custom:
        anchor = QStringLiteral("top left");
        x = QStringLiteral("%1px").arg(m_settings.overlayCustomPos().x());
        y = QStringLiteral("%1px").arg(m_settings.overlayCustomPos().y());
        break;
    }

    QString yuck = QStringLiteral(
        "(defvar azkare-text \"\")\n"
        "(defvar azkare-translation \"\")\n"
        "(defvar azkare-meta \"\")\n\n"
        "(defwidget azkare-widget []\n"
        "  (box :class \"azkare-box\" :orientation \"v\" :space-evenly false\n"
        "    (label :class \"azkare-text\" :text azkare-text :wrap true :justify \"center\")\n"
        "    (label :class \"azkare-translation\" :text azkare-translation :wrap true :justify \"center\"\n"
        "           :visible {azkare-translation != \"\"})\n"
        "    (label :class \"azkare-meta\" :text azkare-meta :visible {azkare-meta != \"\"})))\n\n"
        "(defwindow azkare-window\n"
        "  :monitor @MONITOR@\n"
        "  :geometry (geometry :x \"@X@\" :y \"@Y@\" :width \"60%\" :anchor \"@ANCHOR@\")\n"
        "  :stacking \"bg\"\n"
        "  :exclusive false\n"
        "  :focusable false\n"
        "  (azkare-widget))\n");
    yuck.replace(QLatin1String("@MONITOR@"), QString::number(m_settings.overlayMonitor()));
    yuck.replace(QLatin1String("@X@"), x);
    yuck.replace(QLatin1String("@Y@"), y);
    yuck.replace(QLatin1String("@ANCHOR@"), anchor);

    const QColor c = m_settings.textColor();
    const QString shadow = m_settings.shadowEnabled() ? QStringLiteral("text-shadow: 2px 2px 4px rgba(0,0,0,0.8);") : QString();
    const QString scss = QStringLiteral(
        ".azkare-box { background: transparent; padding: 12px; }\n"
        ".azkare-text { font-family: \"%1\"; font-size: %2pt; color: %3; %4 }\n"
        ".azkare-translation { font-size: %5pt; color: %3; %4 }\n"
        ".azkare-meta { font-size: %6pt; color: %3; %4 }\n")
        .arg(m_settings.fontFamily()).arg(m_settings.fontSize()).arg(c.name(), shadow)
        .arg(qMax(9, m_settings.fontSize() * 55 / 100)).arg(qMax(8, m_settings.fontSize() * 45 / 100));

    for (const auto &pair : {qMakePair(QStringLiteral("eww.yuck"), yuck), qMakePair(QStringLiteral("eww.scss"), scss)}) {
        QFile f(m_configDir + QLatin1Char('/') + pair.first);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            QTextStream(&f) << pair.second;
        else
            qWarning() << "[EwwBridge] cannot write" << f.fileName();
    }
}
