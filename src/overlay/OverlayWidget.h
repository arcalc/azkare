#pragma once
#include <QPoint>
#include <QTextDocument>
#include <QWidget>

#include "../core/AdhkarDatabase.h"
#include "../core/AppSettings.h"

class QGraphicsOpacityEffect;
class QPropertyAnimation;

/**
 * Transparent frameless window that draws the Dhikr above the wallpaper and below
 * normal windows (X11 desktop-type hints, Wayland layer-shell "bottom" layer,
 * Windows WorkerW, macOS desktop window level).
 */
class OverlayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayWidget(AppSettings &settings, QWidget *parent = nullptr);

    void initialize();
    void showEntry(const AdhkarEntry &entry);
    void applySettings();
    void setDragMode(bool enabled);

signals:
    void movedByUser(QPoint newPos);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void rebuildDocument();
    void layoutAndPlace();
    void applyPlatformHints();
    QPoint targetTopLeft(const QRect &screen) const;

    AppSettings &m_settings;
    AdhkarEntry m_entry;
    AdhkarEntry m_pending;
    bool m_hasPending = false;
    QTextDocument m_doc;
    bool m_dragMode = false;
    bool m_dragging = false;
    QPoint m_dragOffset;
    QGraphicsOpacityEffect *m_opacity = nullptr;
    QPropertyAnimation *m_fadeOut = nullptr;
    QPropertyAnimation *m_fadeIn = nullptr;
};
