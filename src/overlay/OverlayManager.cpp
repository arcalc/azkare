#include "OverlayManager.h"

#include "EwwBridge.h"
#include "OverlayWidget.h"
#include "../core/AppSettings.h"

#include <QDebug>

OverlayManager::OverlayManager(AppSettings &settings, QObject *parent) : QObject(parent), m_settings(settings) {}

OverlayManager::~OverlayManager() { teardown(); }

void OverlayManager::teardown()
{
    if (m_eww) { m_eww->close(); delete m_eww; m_eww = nullptr; }
    if (m_widget) { delete m_widget; m_widget = nullptr; }
}

void OverlayManager::applySettings()
{
    if (!m_settings.overlayEnabled()) {
        teardown();
        return;
    }
    bool wantEww = false;
#if defined(Q_OS_LINUX)
    wantEww = m_settings.overlayBackend() == OverlayBackend::Eww;
    if (wantEww && !EwwBridge::isAvailable()) {
        qWarning() << "[OverlayManager] eww not found on PATH - using the built-in overlay";
        wantEww = false;
    }
#endif
    if (wantEww) {
        if (m_widget) { delete m_widget; m_widget = nullptr; }
        if (!m_eww) {
            m_eww = new EwwBridge(m_settings, this);
            m_eww->initialize();
        } else {
            m_eww->reconfigure();
        }
        if (m_last.isValid()) m_eww->showEntry(m_last);
        return;
    }
    if (m_eww) { m_eww->close(); delete m_eww; m_eww = nullptr; }
    if (!m_widget) {
        m_widget = new OverlayWidget(m_settings);
        connect(m_widget, &OverlayWidget::movedByUser, this, [this](QPoint pos) {
            m_settings.setOverlayCustomPos(pos);
            m_settings.setOverlayPosition(OverlayPosition::Custom);
            emit overlayMoved(pos);
        });
        m_widget->initialize();
    } else {
        m_widget->applySettings();
    }
    if (m_last.isValid()) m_widget->showEntry(m_last);
}

void OverlayManager::showEntry(const AdhkarEntry &entry)
{
    if (entry.isValid())
        m_last = entry;
    if (m_eww) m_eww->showEntry(entry);
    else if (m_widget) m_widget->showEntry(entry);
}

void OverlayManager::setDragMode(bool enabled)
{
    if (m_widget) m_widget->setDragMode(enabled);
}
