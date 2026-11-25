#pragma once
#include <QColor>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <Qt>

enum class DisplayMode { OverlayOnly, NotificationsOnly, Both };
enum class OverlayBackend { Native, Eww };
enum class OverlayPosition { TopLeft, TopCenter, TopRight, Center, BottomLeft, BottomCenter, BottomRight, Custom };

/** All user settings, persisted as JSON in the app config directory. */
class AppSettings : public QObject
{
    Q_OBJECT
public:
    explicit AppSettings(QObject *parent = nullptr);

    static QString configFilePath();
    bool load();          ///< overlays values found on disk onto the current ones
    bool save() const;    ///< atomic write
    void resetToDefaults();

    QString language() const { return m_d.language; }
    void setLanguage(const QString &v) { m_d.language = (v == QLatin1String("en")) ? QStringLiteral("en") : QStringLiteral("ar"); }

    DisplayMode displayMode() const { return m_d.mode; }
    void setDisplayMode(DisplayMode m) { m_d.mode = m; }
    bool overlayEnabled() const { return m_d.mode != DisplayMode::NotificationsOnly; }
    bool notificationsEnabled() const { return m_d.mode != DisplayMode::OverlayOnly; }

    OverlayBackend overlayBackend() const { return m_d.backend; }
    void setOverlayBackend(OverlayBackend b) { m_d.backend = b; }
    OverlayPosition overlayPosition() const { return m_d.position; }
    void setOverlayPosition(OverlayPosition p) { m_d.position = p; }
    QPoint overlayCustomPos() const { return m_d.customPos; }
    void setOverlayCustomPos(const QPoint &p) { m_d.customPos = p; }
    int overlayMonitor() const { return m_d.monitor; }
    void setOverlayMonitor(int i) { m_d.monitor = qMax(0, i); }

    int rotationIntervalSecs() const { return m_d.rotationSecs; }
    void setRotationIntervalSecs(int s) { m_d.rotationSecs = qBound(5, s, 86400); }
    int notificationIntervalSecs() const { return m_d.notifSecs; }
    void setNotificationIntervalSecs(int s) { m_d.notifSecs = qBound(30, s, 86400); }

    QString fontFamily() const { return m_d.fontFamily; }
    void setFontFamily(const QString &f) { if (!f.isEmpty()) m_d.fontFamily = f; }
    int fontSize() const { return m_d.fontSize; }
    void setFontSize(int pt) { m_d.fontSize = qBound(8, pt, 96); }
    QColor textColor() const { return m_d.color; }
    void setTextColor(const QColor &c) { if (c.isValid()) m_d.color = c; }
    bool shadowEnabled() const { return m_d.shadow; }
    void setShadowEnabled(bool on) { m_d.shadow = on; }
    Qt::Alignment textAlignment() const { return m_d.align; }
    void setTextAlignment(Qt::Alignment a) { m_d.align = a; }

    QStringList activeCategories() const { return m_d.categories; }
    void setActiveCategories(const QStringList &ids) { m_d.categories = ids; }
    bool shuffleEnabled() const { return m_d.shuffle; }
    void setShuffleEnabled(bool on) { m_d.shuffle = on; }
    bool showSource() const { return m_d.showSource; }
    void setShowSource(bool on) { m_d.showSource = on; }
    bool showTranslation() const { return m_d.showTranslation; }
    void setShowTranslation(bool on) { m_d.showTranslation = on; }

private:
    struct Data {
        QString language = QStringLiteral("ar");
        DisplayMode mode = DisplayMode::Both;
        OverlayBackend backend = OverlayBackend::Native;
        OverlayPosition position = OverlayPosition::BottomCenter;
        QPoint customPos = QPoint(100, 100);
        int monitor = 0;
        int rotationSecs = 300;
        int notifSecs = 1800;
        QString fontFamily = QStringLiteral("Amiri");
        int fontSize = 26;
        QColor color = QColor(255, 255, 255);
        bool shadow = true;
        Qt::Alignment align = Qt::AlignHCenter;
        QStringList categories;   // empty = all
        bool shuffle = false;
        bool showSource = true;
        bool showTranslation = false;
    } m_d;
};
