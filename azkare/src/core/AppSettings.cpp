#include "AppSettings.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ── Helpers: enum ↔ string ────────────────────────────────────────────────────

static QString displayModeToStr(DisplayMode m) {
    switch (m) {
    case DisplayMode::OverlayOnly:       return "overlay_only";
    case DisplayMode::NotificationsOnly: return "notifications_only";
    case DisplayMode::Both:              return "both";
    }
    return "both";
}
static DisplayMode strToDisplayMode(const QString &s) {
    if (s == "overlay_only")       return DisplayMode::OverlayOnly;
    if (s == "notifications_only") return DisplayMode::NotificationsOnly;
    return DisplayMode::Both;
}

static QString backendToStr(OverlayBackend b) {
    return b == OverlayBackend::Eww ? "eww" : "native";
}
static OverlayBackend strToBackend(const QString &s) {
    return s == "eww" ? OverlayBackend::Eww : OverlayBackend::Native;
}

static QString posToStr(OverlayPosition p) {
    switch (p) {
    case OverlayPosition::TopLeft:      return "top_left";
    case OverlayPosition::TopCenter:    return "top_center";
    case OverlayPosition::TopRight:     return "top_right";
    case OverlayPosition::Center:       return "center";
    case OverlayPosition::BottomLeft:   return "bottom_left";
    case OverlayPosition::BottomCenter: return "bottom_center";
    case OverlayPosition::BottomRight:  return "bottom_right";
    case OverlayPosition::Custom:       return "custom";
    }
    return "bottom_center";
}
static OverlayPosition strToPos(const QString &s) {
    if (s == "top_left")      return OverlayPosition::TopLeft;
    if (s == "top_center")    return OverlayPosition::TopCenter;
    if (s == "top_right")     return OverlayPosition::TopRight;
    if (s == "center")        return OverlayPosition::Center;
    if (s == "bottom_left")   return OverlayPosition::BottomLeft;
    if (s == "bottom_center") return OverlayPosition::BottomCenter;
    if (s == "bottom_right")  return OverlayPosition::BottomRight;
    if (s == "custom")        return OverlayPosition::Custom;
    return OverlayPosition::BottomCenter;
}

// ── Constructor ───────────────────────────────────────────────────────────────

AppSettings::AppSettings(QObject *parent) : QObject(parent) {}

// ── Persistence ───────────────────────────────────────────────────────────────

QString AppSettings::configFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + "/settings.json";
}

void AppSettings::load()
{
    QFile f(configFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        qInfo() << "[AppSettings] No config file found; using defaults.";
        return;
    }

    try {
        json root = json::parse(f.readAll().constData());

#define LOAD_STR(key, setter, def) \
    if (root.contains(#key)) setter(QString::fromStdString(root[#key].get<std::string>())); \
    else setter(def);

#define LOAD_BOOL(key, field) \
    if (root.contains(#key)) field = root[#key].get<bool>();

#define LOAD_INT(key, field) \
    if (root.contains(#key)) field = root[#key].get<int>();

        LOAD_STR(language, setLanguage, "ar")
        LOAD_BOOL(autostart, m_autostart)

        if (root.contains("display_mode"))
            setDisplayMode(strToDisplayMode(QString::fromStdString(root["display_mode"].get<std::string>())));
        if (root.contains("overlay_backend"))
            setOverlayBackend(strToBackend(QString::fromStdString(root["overlay_backend"].get<std::string>())));
        if (root.contains("overlay_position"))
            setOverlayPosition(strToPos(QString::fromStdString(root["overlay_position"].get<std::string>())));

        if (root.contains("custom_pos_x") && root.contains("custom_pos_y"))
            m_customPos = {root["custom_pos_x"].get<int>(), root["custom_pos_y"].get<int>()};

        LOAD_INT(monitor, m_monitor)
        LOAD_INT(rotation_secs, m_rotationSecs)

        LOAD_STR(font_family, setFontFamily, "Amiri")
        LOAD_INT(font_size, m_fontSize)
        LOAD_BOOL(shadow, m_shadow)

        if (root.contains("text_color"))
            m_textColor = QColor(QString::fromStdString(root["text_color"].get<std::string>()));

        LOAD_BOOL(notifications_enabled, m_notifEnabled)
        LOAD_INT(notification_secs, m_notifSecs)
        LOAD_BOOL(shuffle, m_shuffle)
        LOAD_BOOL(show_source, m_showSource)
        LOAD_BOOL(show_translation, m_showTranslation)

        if (root.contains("active_categories")) {
            m_activeCategories.clear();
            for (const auto &cat : root["active_categories"])
                m_activeCategories << QString::fromStdString(cat.get<std::string>());
        }

#undef LOAD_STR
#undef LOAD_BOOL
#undef LOAD_INT

    } catch (const json::exception &ex) {
        qWarning() << "[AppSettings] JSON parse error:" << ex.what();
    }
}

void AppSettings::save() const
{
    json root;
    root["language"]              = m_language.toStdString();
    root["autostart"]             = m_autostart;
    root["display_mode"]          = displayModeToStr(m_displayMode).toStdString();
    root["overlay_backend"]       = backendToStr(m_backend).toStdString();
    root["overlay_position"]      = posToStr(m_position).toStdString();
    root["custom_pos_x"]          = m_customPos.x();
    root["custom_pos_y"]          = m_customPos.y();
    root["monitor"]               = m_monitor;
    root["rotation_secs"]         = m_rotationSecs;
    root["font_family"]           = m_fontFamily.toStdString();
    root["font_size"]             = m_fontSize;
    root["shadow"]                = m_shadow;
    root["text_color"]            = m_textColor.name().toStdString();
    root["notifications_enabled"] = m_notifEnabled;
    root["notification_secs"]     = m_notifSecs;
    root["shuffle"]               = m_shuffle;
    root["show_source"]           = m_showSource;
    root["show_translation"]      = m_showTranslation;

    json cats = json::array();
    for (const QString &c : m_activeCategories)
        cats.push_back(c.toStdString());
    root["active_categories"] = cats;

    QFile f(configFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[AppSettings] Cannot write config file.";
        return;
    }
    f.write(QByteArray::fromStdString(root.dump(2)));
}

// ── Accessors ─────────────────────────────────────────────────────────────────

QString AppSettings::language() const { return m_language; }
void AppSettings::setLanguage(const QString &l) { m_language = l; }

bool AppSettings::autostartEnabled() const { return m_autostart; }
void AppSettings::setAutostartEnabled(bool on) { m_autostart = on; }

DisplayMode AppSettings::displayMode() const { return m_displayMode; }
void AppSettings::setDisplayMode(DisplayMode m) { m_displayMode = m; }

OverlayBackend AppSettings::overlayBackend() const { return m_backend; }
void AppSettings::setOverlayBackend(OverlayBackend b) { m_backend = b; }

OverlayPosition AppSettings::overlayPosition() const { return m_position; }
void AppSettings::setOverlayPosition(OverlayPosition p) { m_position = p; }

QPoint AppSettings::overlayCustomPos() const { return m_customPos; }
void AppSettings::setOverlayCustomPos(const QPoint &pos) { m_customPos = pos; }

int AppSettings::overlayMonitor() const { return m_monitor; }
void AppSettings::setOverlayMonitor(int idx) { m_monitor = idx; }

int AppSettings::rotationIntervalSecs() const { return m_rotationSecs; }
void AppSettings::setRotationIntervalSecs(int s) { m_rotationSecs = s; }

QString AppSettings::fontFamily() const { return m_fontFamily; }
void AppSettings::setFontFamily(const QString &f) { m_fontFamily = f; }

int AppSettings::fontSize() const { return m_fontSize; }
void AppSettings::setFontSize(int pt) { m_fontSize = pt; }

QColor AppSettings::textColor() const { return m_textColor; }
void AppSettings::setTextColor(const QColor &c) { m_textColor = c; }

bool AppSettings::shadowEnabled() const { return m_shadow; }
void AppSettings::setShadowEnabled(bool on) { m_shadow = on; }

Qt::Alignment AppSettings::textAlignment() const { return m_alignment; }
void AppSettings::setTextAlignment(Qt::Alignment a) { m_alignment = a; }

bool AppSettings::notificationsEnabled() const { return m_notifEnabled; }
void AppSettings::setNotificationsEnabled(bool on) { m_notifEnabled = on; }

int AppSettings::notificationIntervalSecs() const { return m_notifSecs; }
void AppSettings::setNotificationIntervalSecs(int s) { m_notifSecs = s; }

QStringList AppSettings::activeCategories() const { return m_activeCategories; }
void AppSettings::setActiveCategories(const QStringList &ids) { m_activeCategories = ids; }

bool AppSettings::shuffleEnabled() const { return m_shuffle; }
void AppSettings::setShuffleEnabled(bool on) { m_shuffle = on; }

bool AppSettings::showSource() const { return m_showSource; }
void AppSettings::setShowSource(bool on) { m_showSource = on; }

bool AppSettings::showTranslation() const { return m_showTranslation; }
void AppSettings::setShowTranslation(bool on) { m_showTranslation = on; }
