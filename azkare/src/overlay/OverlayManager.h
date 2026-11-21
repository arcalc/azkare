#pragma once

#include <QObject>
#include "../core/AdhkarDatabase.h"
#include "../core/AppSettings.h"

class OverlayWidget;   // forward — platform-specific subclass
class EwwBridge;       // forward — Linux eww IPC

/**
 * Top-level overlay controller.  Decides which backend to use,
 * constructs the right renderer, and forwards showEntry() calls.
 */
class OverlayManager : public QObject
{
    Q_OBJECT

public:
    explicit OverlayManager(AppSettings    &settings,
                            AdhkarDatabase &db,
                            QObject        *parent = nullptr);
    ~OverlayManager() override;

    /** Create the overlay window / eww config. */
    void initialize();

    /** Display a new entry (smooth fade transition). */
    void showEntry(const AdhkarEntry &entry);

    /** Hide the overlay entirely (NotificationsOnly mode). */
    void setVisible(bool visible);

    /** Update appearance from current settings (font, color, position, etc.) */
    void applySettings();

signals:
    void overlayMoved(QPoint newPos); ///< Emitted when user drags overlay

private:
    AppSettings    &m_settings;
    AdhkarDatabase &m_db;

    OverlayWidget *m_widget  = nullptr;
    EwwBridge     *m_eww     = nullptr;
};
