#pragma once
#include <QObject>
#include <QPoint>

#include "../core/AdhkarDatabase.h"

class AppSettings;
class OverlayWidget;
class EwwBridge;

/** Owns the active overlay backend (native Qt window or eww) and follows the settings. */
class OverlayManager : public QObject
{
    Q_OBJECT
public:
    OverlayManager(AppSettings &settings, QObject *parent = nullptr);
    ~OverlayManager() override;

    void initialize() { applySettings(); }
    void showEntry(const AdhkarEntry &entry);
    void applySettings();          ///< (re)creates / tears down the backend as the mode requires
    void setDragMode(bool enabled);

signals:
    void overlayMoved(QPoint newPos);

private:
    void teardown();

    AppSettings &m_settings;
    OverlayWidget *m_widget = nullptr;
    EwwBridge *m_eww = nullptr;
    AdhkarEntry m_last;
};
