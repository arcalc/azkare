/**
 * Azkare (أذكاري) — Cross-Platform Desktop Adhkar & Dua Engine
 * Entry point: initializes Qt, loads settings and the Adhkar database,
 * then brings up the system tray, overlay, and notification scheduler.
 *
 * Author : Mahmoud El Sheikh (arcalc)
 * License: MIT
 */

#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QStandardPaths>
#include <QTranslator>

#include "core/AdhkarDatabase.h"
#include "core/AppSettings.h"
#include "core/Scheduler.h"
#include "core/Updater.h"
#include "overlay/OverlayManager.h"
#include "notifications/NotificationDispatcher.h"
#include "platform/AutostartManager.h"
#include "platform/TrayController.h"
#include "ui/SettingsWindow.h"

// ── Font registration ────────────────────────────────────────────────────────
static void registerBundledFonts()
{
    // Fonts are embedded in Qt resources under :/fonts/
    const QStringList fontFiles = {
        ":/fonts/Amiri-Regular.ttf",
        ":/fonts/Amiri-Bold.ttf",
        ":/fonts/ScheherazadeNew-Regular.ttf",
        ":/fonts/ScheherazadeNew-Bold.ttf",
        ":/fonts/NotoNaskhArabic-Regular.ttf",
        ":/fonts/Cairo-Regular.ttf",
        ":/fonts/Cairo-Bold.ttf",
        ":/fonts/IBMPlexSansArabic-Regular.ttf",
        ":/fonts/ArefRuqaa-Regular.ttf",
    };
    for (const QString &path : fontFiles) {
        int id = QFontDatabase::addApplicationFont(path);
        if (id == -1)
            qWarning() << "[Azkare] Could not load bundled font:" << path;
    }
}

// ── Locale / translator ───────────────────────────────────────────────────────
static void installTranslator(QApplication &app, const QString &langCode)
{
    static QTranslator translator;
    const QString qmPath = QString(":/i18n/%1.qm").arg(langCode);
    if (translator.load(qmPath))
        app.installTranslator(&translator);
}

int main(int argc, char *argv[])
{
    // High-DPI support
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Azkare");
    app.setApplicationDisplayName("أذكاري");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("arcalc");
    app.setOrganizationDomain("io.github.arcalc.azkare");

    // Keep running when the last window is closed (tray app)
    app.setQuitOnLastWindowClosed(false);

    // ── Settings (loads persisted JSON config) ────────────────────────────
    AppSettings settings;
    settings.load();

    // ── Translator ────────────────────────────────────────────────────────
    installTranslator(app, settings.language());

    // ── Fonts ─────────────────────────────────────────────────────────────
    registerBundledFonts();

    // ── Adhkar database ───────────────────────────────────────────────────
    AdhkarDatabase db;
    if (!db.load(":/data/adhkar.json")) {
        qCritical() << "[Azkare] Fatal: failed to load Adhkar database.";
        return 1;
    }
    db.setActiveCategories(settings.activeCategories());
    db.setShuffleEnabled(settings.shuffleEnabled());

    // ── Desktop overlay ───────────────────────────────────────────────────
    OverlayManager overlay(settings, db);
    overlay.initialize();

    // ── Notification dispatcher ───────────────────────────────────────────
    NotificationDispatcher notifier(settings, db);
    notifier.initialize();

    // ── Scheduler: drives overlay rotation and notification cadence ───────
    Scheduler scheduler(settings, db, overlay, notifier);
    scheduler.start();

    // ── System tray ───────────────────────────────────────────────────────
    TrayController tray(settings, scheduler, overlay, notifier);
    tray.show();

    // ── Settings window (created lazily, shown on tray action) ───────────
    SettingsWindow settingsWin(settings, db, overlay, notifier, scheduler, tray);

    QObject::connect(&tray, &TrayController::openSettingsRequested,
                     &settingsWin, &SettingsWindow::showAndRaise);

    QObject::connect(&tray, &TrayController::nextDhikrRequested,
                     &scheduler, &Scheduler::forceNext);

    QObject::connect(&tray, &TrayController::pauseResumeRequested,
                     &scheduler, &Scheduler::togglePause);

    // ── Update checker (runs in background, non-blocking) ─────────────────
    Updater updater("arcalc", "azkare", app.applicationVersion());
    QObject::connect(&updater, &Updater::updateAvailable,
                     &tray, &TrayController::showUpdateNotification);
    updater.checkAsync();

    return app.exec();
}
