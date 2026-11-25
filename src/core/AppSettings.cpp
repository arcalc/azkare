#include "AppSettings.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDebug>

namespace {
struct ModeName { DisplayMode v; const char *s; };
const ModeName kModes[] = {{DisplayMode::OverlayOnly, "overlay"}, {DisplayMode::NotificationsOnly, "notify"}, {DisplayMode::Both, "both"}};
struct PosName { OverlayPosition v; const char *s; };
const PosName kPositions[] = {
    {OverlayPosition::TopLeft, "top_left"}, {OverlayPosition::TopCenter, "top_center"}, {OverlayPosition::TopRight, "top_right"},
    {OverlayPosition::Center, "center"}, {OverlayPosition::BottomLeft, "bottom_left"},
    {OverlayPosition::BottomCenter, "bottom_center"}, {OverlayPosition::BottomRight, "bottom_right"}, {OverlayPosition::Custom, "custom"}};

QString modeToStr(DisplayMode m) { for (const auto &e : kModes) if (e.v == m) return QLatin1String(e.s); return QStringLiteral("both"); }
DisplayMode strToMode(const QString &s) { for (const auto &e : kModes) if (s == QLatin1String(e.s)) return e.v; return DisplayMode::Both; }
QString posToStr(OverlayPosition p) { for (const auto &e : kPositions) if (e.v == p) return QLatin1String(e.s); return QStringLiteral("bottom_center"); }
OverlayPosition strToPos(const QString &s) { for (const auto &e : kPositions) if (s == QLatin1String(e.s)) return e.v; return OverlayPosition::BottomCenter; }

QString alignToStr(Qt::Alignment a) {
    if (a & Qt::AlignLeft) return QStringLiteral("left");
    if (a & Qt::AlignRight) return QStringLiteral("right");
    return QStringLiteral("center");
}
Qt::Alignment strToAlign(const QString &s) {
    if (s == QLatin1String("left")) return Qt::AlignLeft;
    if (s == QLatin1String("right")) return Qt::AlignRight;
    return Qt::AlignHCenter;
}
} // namespace

AppSettings::AppSettings(QObject *parent) : QObject(parent) {}

void AppSettings::resetToDefaults() { m_d = Data(); }

QString AppSettings::configFilePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/settings.json");
}

bool AppSettings::load()
{
    QFile f(configFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[AppSettings] ignoring invalid settings file:" << err.errorString();
        return false;
    }
    const QJsonObject o = doc.object();
    auto has = [&o](const char *k) { return o.contains(QLatin1String(k)); };
    auto val = [&o](const char *k) { return o.value(QLatin1String(k)); };

    if (has("language")) setLanguage(val("language").toString());
    if (has("display_mode")) m_d.mode = strToMode(val("display_mode").toString());
    if (has("overlay_backend")) m_d.backend = val("overlay_backend").toString() == QLatin1String("eww") ? OverlayBackend::Eww : OverlayBackend::Native;
    if (has("overlay_position")) m_d.position = strToPos(val("overlay_position").toString());
    if (has("custom_pos_x") && has("custom_pos_y")) m_d.customPos = QPoint(val("custom_pos_x").toInt(), val("custom_pos_y").toInt());
    if (has("monitor")) setOverlayMonitor(val("monitor").toInt());
    if (has("rotation_secs")) setRotationIntervalSecs(val("rotation_secs").toInt());
    if (has("notification_secs")) setNotificationIntervalSecs(val("notification_secs").toInt());
    if (has("font_family")) setFontFamily(val("font_family").toString());
    if (has("font_size")) setFontSize(val("font_size").toInt());
    if (has("text_color")) setTextColor(QColor(val("text_color").toString()));
    if (has("shadow")) m_d.shadow = val("shadow").toBool();
    if (has("alignment")) m_d.align = strToAlign(val("alignment").toString());
    if (has("shuffle")) m_d.shuffle = val("shuffle").toBool();
    if (has("show_source")) m_d.showSource = val("show_source").toBool();
    if (has("show_translation")) m_d.showTranslation = val("show_translation").toBool();
    if (has("active_categories")) {
        m_d.categories.clear();
        for (const QJsonValue &v : val("active_categories").toArray())
            m_d.categories << v.toString();
    }
    return true;
}

bool AppSettings::save() const
{
    QJsonObject o;
    o.insert(QStringLiteral("language"), m_d.language);
    o.insert(QStringLiteral("display_mode"), modeToStr(m_d.mode));
    o.insert(QStringLiteral("overlay_backend"), m_d.backend == OverlayBackend::Eww ? QStringLiteral("eww") : QStringLiteral("native"));
    o.insert(QStringLiteral("overlay_position"), posToStr(m_d.position));
    o.insert(QStringLiteral("custom_pos_x"), m_d.customPos.x());
    o.insert(QStringLiteral("custom_pos_y"), m_d.customPos.y());
    o.insert(QStringLiteral("monitor"), m_d.monitor);
    o.insert(QStringLiteral("rotation_secs"), m_d.rotationSecs);
    o.insert(QStringLiteral("notification_secs"), m_d.notifSecs);
    o.insert(QStringLiteral("font_family"), m_d.fontFamily);
    o.insert(QStringLiteral("font_size"), m_d.fontSize);
    o.insert(QStringLiteral("text_color"), m_d.color.name());
    o.insert(QStringLiteral("shadow"), m_d.shadow);
    o.insert(QStringLiteral("alignment"), alignToStr(m_d.align));
    o.insert(QStringLiteral("shuffle"), m_d.shuffle);
    o.insert(QStringLiteral("show_source"), m_d.showSource);
    o.insert(QStringLiteral("show_translation"), m_d.showTranslation);
    o.insert(QStringLiteral("active_categories"), QJsonArray::fromStringList(m_d.categories));

    QDir().mkpath(QFileInfo(configFilePath()).absolutePath());
    QSaveFile f(configFilePath());
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[AppSettings] cannot write" << configFilePath();
        return false;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return f.commit();
}
