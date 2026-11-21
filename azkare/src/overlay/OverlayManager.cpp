#include "OverlayManager.h"
#include "OverlayWidget.h"
#include "EwwBridge.h"

#include "../core/AppSettings.h"
#include "../core/AdhkarDatabase.h"

#include <QDebug>

OverlayManager::OverlayManager(AppSettings    &settings,
                               AdhkarDatabase &db,
                               QObject        *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_db(db)
{}

OverlayManager::~OverlayManager()
{
    if (m_eww)    m_eww->close();
    if (m_widget) m_widget->hide();
}

void OverlayManager::initialize()
{
    // Skip overlay entirely in Notifications-only mode
    if (m_settings.displayMode() == DisplayMode::NotificationsOnly) {
        qInfo() << "[OverlayManager] Notifications-only mode — no overlay.";
        return;
    }

#if defined(Q_OS_LINUX)
    if (m_settings.overlayBackend() == OverlayBackend::Eww) {
        if (EwwBridge::isAvailable()) {
            m_eww = new EwwBridge(m_settings, this);
            m_eww->initialize();
            qInfo() << "[OverlayManager] eww backend active.";
            return;
        } else {
            qWarning() << "[OverlayManager] eww not found on PATH — falling back to native.";
        }
    }
#endif

    // Native Qt overlay (all platforms)
    m_widget = new OverlayWidget(m_settings);
    connect(m_widget, &OverlayWidget::movedByUser, this, [this](QPoint pos) {
        m_settings.setOverlayCustomPos(pos);
        m_settings.setOverlayPosition(OverlayPosition::Custom);
        emit overlayMoved(pos);
    });
    m_widget->initialize();
    qInfo() << "[OverlayManager] Native overlay active.";
}

void OverlayManager::showEntry(const AdhkarEntry &entry)
{
    if (m_settings.displayMode() == DisplayMode::NotificationsOnly) return;

    if (m_eww)    { m_eww->showEntry(entry);    return; }
    if (m_widget) { m_widget->showEntry(entry);  return; }
}

void OverlayManager::setVisible(bool visible)
{
    if (m_widget) m_widget->setVisible(visible);
}

void OverlayManager::applySettings()
{
    if (m_widget) m_widget->applySettings();

    if (m_eww) {
        // Re-write config and reload eww
        m_eww->close();
        m_eww->initialize();
    }
}
