#include "TrayController.h"
#include "../core/AppSettings.h"
#include "../core/Scheduler.h"
#include "../core/Updater.h"
#include <QIcon>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

TrayController::TrayController(AppSettings            &settings,
                               Scheduler              &scheduler,
                               OverlayManager         &overlay,
                               NotificationDispatcher &notifier,
                               QObject                *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_scheduler(scheduler)
    , m_overlay(overlay)
    , m_notifier(notifier)
{}

void TrayController::show()
{
    m_tray = new QSystemTrayIcon(this);

    // Try theme icon first, then fallback to embedded
    QIcon icon = QIcon::fromTheme("azkare");
    if (icon.isNull())
        icon = QIcon(":/icons/azkare.svg");
    m_tray->setIcon(icon);
    m_tray->setToolTip("أذكاري — Azkare");

    buildMenu();
    m_tray->setContextMenu(m_menu);
    m_tray->setVisible(true);

    connect(m_tray, &QSystemTrayIcon::activated,
            this,   &TrayController::onTrayActivated);

    connect(&m_scheduler, &Scheduler::pauseStateChanged,
            this,          &TrayController::updatePauseLabel);
}

void TrayController::buildMenu()
{
    m_menu = new QMenu;

    // Header (non-interactive)
    QAction *header = m_menu->addAction("أذكاري — Azkare");
    header->setEnabled(false);
    QFont boldFont = header->font();
    boldFont.setBold(true);
    header->setFont(boldFont);

    m_menu->addSeparator();

    m_actNext = m_menu->addAction(QIcon::fromTheme("go-next"), "الذكر التالي");
    connect(m_actNext, &QAction::triggered, this, &TrayController::nextDhikrRequested);

    m_actPause = m_menu->addAction(QIcon::fromTheme("media-playback-pause"), "إيقاف مؤقت");
    connect(m_actPause, &QAction::triggered, this, &TrayController::pauseResumeRequested);

    m_menu->addSeparator();

    m_actSettings = m_menu->addAction(QIcon::fromTheme("preferences-system"), "الإعدادات");
    connect(m_actSettings, &QAction::triggered, this, &TrayController::openSettingsRequested);

    m_menu->addSeparator();

    // Update action — hidden until an update is detected
    m_actUpdate = m_menu->addAction(QIcon::fromTheme("system-software-update"), "");
    m_actUpdate->setVisible(false);

    m_actQuit = m_menu->addAction(QIcon::fromTheme("application-exit"), "إنهاء");
    connect(m_actQuit, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void TrayController::updatePauseLabel(bool paused)
{
    if (!m_actPause) return;
    m_actPause->setText(paused ? "استئناف" : "إيقاف مؤقت");
    QIcon icon = QIcon::fromTheme(paused ? "media-playback-start" : "media-playback-pause");
    m_actPause->setIcon(icon);
}

void TrayController::showUpdateNotification(const ReleaseInfo &info)
{
    if (!m_actUpdate) return;
    m_actUpdate->setText(QString("تحديث جديد: %1").arg(info.tag));
    m_actUpdate->setVisible(true);

    QString url = info.downloadUrl;
    connect(m_actUpdate, &QAction::triggered, this, [url]() {
        QDesktopServices::openUrl(QUrl(url));
    });

    // Also show a tray balloon
    if (m_tray) {
        m_tray->showMessage(
            "أذكاري — تحديث جديد",
            QString("الإصدار %1 متاح الآن. انقر للتنزيل.").arg(info.tag),
            QSystemTrayIcon::Information,
            8000
        );
    }
}

void TrayController::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    // Middle-click or double-click: advance Dhikr
    if (reason == QSystemTrayIcon::MiddleClick ||
        reason == QSystemTrayIcon::DoubleClick)
    {
        emit nextDhikrRequested();
    }
}
