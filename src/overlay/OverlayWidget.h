#pragma once

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include "../core/AdhkarDatabase.h"
#include "../core/AppSettings.h"

/**
 * Transparent, frameless Qt window that renders Adhkar text above the
 * desktop wallpaper and below all normal application windows.
 *
 * Platform-specific z-ordering is applied in initialize():
 *   Linux X11   — _NET_WM_WINDOW_TYPE_DESKTOP + XLowerWindow
 *   Linux Wayland — via platform plugin hints (gtk-layer-shell / wlr-layer-shell)
 *   Windows     — WorkerW injection or WS_EX_LAYERED desktop window
 *   macOS       — NSWindow at kCGDesktopWindowLevel (bridged via native helper)
 */
class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget(AppSettings &settings, QWidget *parent = nullptr);

    /** Apply platform-specific desktop-level z-order. */
    void initialize();

    /** Fade in a new Adhkar entry. */
    void showEntry(const AdhkarEntry &entry);

    /** Reflect current settings (font, color, position). */
    void applySettings();

    /** Enter/leave interactive drag mode. */
    void setDragMode(bool enabled);

signals:
    void movedByUser(QPoint newPos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void computePosition();
    void applyX11DesktopHints();
    void applyWindowsDesktopLayer();
    void applyMacOSDesktopLevel();

    // ── State ─────────────────────────────────────────────────────────────
    AppSettings   &m_settings;
    AdhkarEntry    m_entry;
    bool           m_dragMode  = false;
    bool           m_dragging  = false;
    QPoint         m_dragStart;

    // ── Animation ─────────────────────────────────────────────────────────
    QGraphicsOpacityEffect *m_opacity  = nullptr;
    QPropertyAnimation     *m_fadeOut  = nullptr;
    QPropertyAnimation     *m_fadeIn   = nullptr;
    AdhkarEntry             m_pending;   // shown after fade-out completes
    bool                    m_hasPending = false;
};
