#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QPoint>
#include <QFont>

// Which display mode is active
enum class DisplayMode {
    OverlayOnly,
    NotificationsOnly,
    Both
};

// Which rendering backend to use on Linux
enum class OverlayBackend {
    Native, ///< Built-in C++ renderer (default, all platforms)
    Eww     ///< ElKowar's Wacky Widgets (Linux only)
};

// Preset anchor for overlay position
enum class OverlayPosition {
    TopLeft, TopCenter, TopRight,
    Center,
    BottomLeft, BottomCenter, BottomRight,
    Custom
};

/**
 * All user-configurable settings, persisted as JSON in the
 * platform's standard app config directory.
 */
class AppSettings : public QObject
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);

    // ── Persistence ───────────────────────────────────────────────────────
    void load();
    void save() const;

    // ── General ───────────────────────────────────────────────────────────
    QString     language() const;        ///< "ar" or "en"
    void        setLanguage(const QString &lang);

    bool        autostartEnabled() const;
    void        setAutostartEnabled(bool on);

    // ── Display mode ──────────────────────────────────────────────────────
    DisplayMode displayMode() const;
    void        setDisplayMode(DisplayMode m);

    // ── Overlay ───────────────────────────────────────────────────────────
    OverlayBackend   overlayBackend() const;
    void             setOverlayBackend(OverlayBackend b);

    OverlayPosition  overlayPosition() const;
    void             setOverlayPosition(OverlayPosition p);

    QPoint           overlayCustomPos() const;
    void             setOverlayCustomPos(const QPoint &pos);

    int              overlayMonitor() const;
    void             setOverlayMonitor(int idx);

    int              rotationIntervalSecs() const;
    void             setRotationIntervalSecs(int secs);

    // ── Appearance ────────────────────────────────────────────────────────
    QString     fontFamily() const;
    void        setFontFamily(const QString &family);

    int         fontSize() const;
    void        setFontSize(int pt);

    QColor      textColor() const;
    void        setTextColor(const QColor &c);

    bool        shadowEnabled() const;
    void        setShadowEnabled(bool on);

    Qt::Alignment textAlignment() const;
    void          setTextAlignment(Qt::Alignment a);

    // ── Notifications ─────────────────────────────────────────────────────
    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool on);

    int  notificationIntervalSecs() const;
    void setNotificationIntervalSecs(int secs);

    // ── Content ───────────────────────────────────────────────────────────
    QStringList activeCategories() const;
    void        setActiveCategories(const QStringList &ids);

    bool shuffleEnabled() const;
    void setShuffleEnabled(bool on);

    bool showSource() const;
    void setShowSource(bool on);

    bool showTranslation() const;
    void setShowTranslation(bool on);

signals:
    void settingsChanged();

private:
    QString configFilePath() const;

    // ── Defaults ──────────────────────────────────────────────────────────
    QString        m_language        = "ar";
    bool           m_autostart       = false;

    DisplayMode    m_displayMode     = DisplayMode::Both;
    OverlayBackend m_backend         = OverlayBackend::Native;
    OverlayPosition m_position       = OverlayPosition::BottomCenter;
    QPoint         m_customPos       = {100, 100};
    int            m_monitor         = 0;
    int            m_rotationSecs    = 300; // 5 min

    QString        m_fontFamily      = "Amiri";
    int            m_fontSize        = 20;
    QColor         m_textColor       = QColor(255, 255, 255);
    bool           m_shadow          = true;
    Qt::Alignment  m_alignment       = Qt::AlignCenter;

    bool           m_notifEnabled    = true;
    int            m_notifSecs       = 1800; // 30 min

    QStringList    m_activeCategories; // empty = all
    bool           m_shuffle         = false;
    bool           m_showSource      = true;
    bool           m_showTranslation = false;
};
