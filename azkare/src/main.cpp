/**
 * Azkare (أذكاري) — Cross-Platform Desktop Adhkar & Dua Engine
 * Entry point: full CLI parser + GUI launcher.
 *
 * Usage:
 *   azkare                    — start overlay + tray
 *   azkare --start            — same as above
 *   azkare --settings         — open settings GUI
 *   azkare --next             — rotate to next Dhikr (sends to running instance)
 *   azkare --mode <overlay|notify|both>
 *   azkare --autostart <on|off>
 *   azkare --update           — pull & rebuild via install.sh / install.ps1
 *   azkare --uninstall        — remove binary + autostart + optionally config
 *   azkare --status           — print running status and version
 *   azkare --version          — print version
 *   azkare --help             — print bilingual help
 *
 * Author : Mahmoud El Sheikh (arcalc)
 * License: MIT
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFontDatabase>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QTranslator>
#include <QTimer>
#include <iostream>

#include "core/AdhkarDatabase.h"
#include "core/AppSettings.h"
#include "core/Scheduler.h"
#include "core/Updater.h"
#include "overlay/OverlayManager.h"
#include "notifications/NotificationDispatcher.h"
#include "platform/AutostartManager.h"
#include "platform/TrayController.h"
#include "ui/SettingsWindow.h"

#include "Version.h"
// Version constants come from cmake-generated Version.h
static constexpr const char *IPC_SOCKET     = "azkare-ipc";

// ── Font registration ─────────────────────────────────────────────────────────
static void registerBundledFonts()
{
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
        if (QFontDatabase::addApplicationFont(path) == -1)
            qWarning() << "[Azkare] Could not load font:" << path;
    }
}

// ── Translator ────────────────────────────────────────────────────────────────
static void installTranslator(QApplication &app, const QString &langCode)
{
    static QTranslator translator;
    if (translator.load(QString(":/i18n/%1.qm").arg(langCode)))
        app.installTranslator(&translator);
}

// ── IPC: send command to already-running instance ────────────────────────────
static bool sendIpcCommand(const QString &cmd)
{
    QLocalSocket sock;
    sock.connectToServer(IPC_SOCKET);
    if (!sock.waitForConnected(500)) return false;
    sock.write((cmd + "\n").toUtf8());
    sock.flush();
    sock.disconnectFromServer();
    return true;
}

// ── CLI helpers ───────────────────────────────────────────────────────────────
static void printHelp()
{
    std::cout <<
R"(أذكاري — Azkare v)" AZKARE_VERSION R"(
Authentic Adhkar & Duas on your desktop | أذكار وأدعية على سطح مكتبك

Usage / الاستخدام:
  azkare                        Start overlay + system tray
  azkare --settings             Open settings window
  azkare --next                 Rotate to the next Dhikr immediately
  azkare --mode <MODE>          Set display mode:
                                  overlay  — desktop text only
                                  notify   — notifications only
                                  both     — overlay + notifications
  azkare --autostart <on|off>   Enable/disable launch at login
  azkare --update               Pull latest source & rebuild
  azkare --uninstall            Remove Azkare from this system
  azkare --status               Show running status
  azkare --version              Print version
  azkare --help                 Show this help

Sources / المصادر:
  Dorar.net · IslamWeb · IslamQA · 500+ verified Adhkar

GitHub: https://github.com/arcalc/azkare
)" << std::endl;
}

static void printVersion()
{
    std::cout << AZKARE_VERSION_FULL << std::endl;
}

static void printStatus(bool running)
{
    std::cout << AZKARE_VERSION_FULL << "\n"
              << "Status: " << (running ? "running" : "not running") << "\n"
              << "Config: "
              << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).toStdString()
              << "/settings.json\n";
}

// ── Self-update: invokes install.sh / install.ps1 ────────────────────────────
static int runUpdate()
{
    std::cout << "[أذكاري] Checking for updates...\n";

#if defined(Q_OS_WIN)
    QString script = QDir::homePath() + "/.local/share/azkare-src/install.ps1";
    if (!QFile::exists(script))
        script = QStandardPaths::findExecutable("powershell");
    QProcess p;
    p.setProgram("powershell");
    p.setArguments({"-ExecutionPolicy", "Bypass", "-File", script, "-Update"});
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start();
    p.waitForFinished(-1);
    return p.exitCode();
#else
    // Find install.sh — prefer local source copy, fall back to curl
    QString script = QDir::homePath() + "/.local/share/azkare-src/install.sh";
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    if (QFile::exists(script)) {
        p.setProgram("/bin/bash");
        p.setArguments({script, "--update"});
    } else {
        // Re-download and run from GitHub
        p.setProgram("/bin/bash");
        p.setArguments({"-c",
            "curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash -s -- --update"});
    }
    p.start();
    p.waitForFinished(-1);
    return p.exitCode();
#endif
}

// ── Self-uninstall ────────────────────────────────────────────────────────────
static int runUninstall()
{
#if defined(Q_OS_WIN)
    QProcess p;
    QString script = QDir::homePath() + "/.local/share/azkare-src/install.ps1";
    p.setProgram("powershell");
    p.setArguments({"-ExecutionPolicy", "Bypass", "-File", script, "-Uninstall"});
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start();
    p.waitForFinished(-1);
    return p.exitCode();
#else
    QString script = QDir::homePath() + "/.local/share/azkare-src/install.sh";
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    if (QFile::exists(script)) {
        p.setProgram("/bin/bash");
        p.setArguments({script, "--uninstall"});
    } else {
        p.setProgram("/bin/bash");
        p.setArguments({"-c",
            "curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash -s -- --uninstall"});
    }
    p.start();
    p.waitForFinished(-1);
    return p.exitCode();
#endif
}

// ═════════════════════════════════════════════════════════════════════════════
int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Azkare");
    app.setApplicationDisplayName("أذكاري");
    app.setApplicationVersion(AZKARE_VERSION);
    app.setOrganizationName("arcalc");
    app.setOrganizationDomain("io.github.arcalc.azkare");
    app.setQuitOnLastWindowClosed(false);

    // ── CLI parser ────────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription("أذكاري — Azkare: Adhkar & Duas on your desktop");
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    parser.addOption({"start",     "Start overlay and system tray (default)"});
    parser.addOption({"settings",  "Open the settings window"});
    parser.addOption({"next",      "Rotate to the next Dhikr"});
    parser.addOption({{"v","version"}, "Print version and exit"});
    parser.addOption({"status",    "Print running status"});
    parser.addOption({{"h","help"}, "Print help and exit"});
    parser.addOption({"update",    "Pull latest source and rebuild"});
    parser.addOption({"uninstall", "Remove Azkare from this system"});
    parser.addOption({"mode",      "Display mode: overlay|notify|both", "mode"});
    parser.addOption({"autostart", "Enable/disable autostart: on|off",  "state"});

    parser.process(app);

    // ── Pure CLI commands (no GUI needed) ─────────────────────────────────
    if (parser.isSet("help"))    { printHelp();    return 0; }
    if (parser.isSet("version")) { printVersion(); return 0; }
    if (parser.isSet("update"))  { return runUpdate(); }
    if (parser.isSet("uninstall")) { return runUninstall(); }

    if (parser.isSet("autostart")) {
        QString state = parser.value("autostart").toLower();
        if (state != "on" && state != "off") {
            std::cerr << "Error: --autostart requires 'on' or 'off'\n";
            return 1;
        }
        AutostartManager am;
        bool ok = am.setEnabled(state == "on");
        std::cout << "Autostart " << (state == "on" ? "enabled" : "disabled")
                  << (ok ? "" : " (failed — check permissions)") << "\n";
        return ok ? 0 : 1;
    }

    // ── Commands that forward to a running instance ────────────────────────
    if (parser.isSet("next")) {
        if (!sendIpcCommand("next")) {
            std::cerr << "Azkare is not running. Start it first with: azkare\n";
            return 1;
        }
        std::cout << "→ Next Dhikr\n";
        return 0;
    }

    if (parser.isSet("mode")) {
        QString m = parser.value("mode").toLower();
        if (m != "overlay" && m != "notify" && m != "both") {
            std::cerr << "Error: --mode requires overlay|notify|both\n";
            return 1;
        }
        if (!sendIpcCommand("mode:" + m)) {
            // Not running — set in config and exit
            AppSettings s; s.load();
            if (m == "overlay") s.setDisplayMode(DisplayMode::OverlayOnly);
            else if (m == "notify") s.setDisplayMode(DisplayMode::NotificationsOnly);
            else s.setDisplayMode(DisplayMode::Both);
            s.save();
            std::cout << "Mode set to '" << m.toStdString() << "' (takes effect on next start)\n";
        } else {
            std::cout << "Mode switched to '" << m.toStdString() << "'\n";
        }
        return 0;
    }

    // ── Status ────────────────────────────────────────────────────────────
    if (parser.isSet("status")) {
        bool running = sendIpcCommand("ping");
        printStatus(running);
        return 0;
    }

    // ── Single-instance guard via QLocalServer ─────────────────────────────
    // Try connecting — if another instance is already running, forward --settings
    {
        QLocalSocket probe;
        probe.connectToServer(IPC_SOCKET);
        if (probe.waitForConnected(300)) {
            if (parser.isSet("settings"))
                probe.write("settings\n");
            probe.flush();
            probe.disconnectFromServer();
            std::cout << "Azkare is already running.\n";
            return 0;
        }
    }

    // ── Full GUI launch ────────────────────────────────────────────────────
    AppSettings settings;
    settings.load();

    installTranslator(app, settings.language());
    registerBundledFonts();

    AdhkarDatabase db;
    if (!db.load(":/data/adhkar.json")) {
        std::cerr << "[Azkare] Fatal: failed to load Adhkar database.\n";
        return 1;
    }
    db.setActiveCategories(settings.activeCategories());
    db.setShuffleEnabled(settings.shuffleEnabled());

    OverlayManager overlay(settings, db);
    overlay.initialize();

    NotificationDispatcher notifier(settings, db);
    notifier.initialize();

    Scheduler scheduler(settings, db, overlay, notifier);
    scheduler.start();

    TrayController tray(settings, scheduler, overlay, notifier);
    tray.show();

    SettingsWindow settingsWin(settings, db, overlay, notifier, scheduler, tray);

    // Open settings immediately if --settings was passed at launch
    if (parser.isSet("settings"))
        QTimer::singleShot(0, &settingsWin, &SettingsWindow::showAndRaise);

    QObject::connect(&tray, &TrayController::openSettingsRequested,
                     &settingsWin, &SettingsWindow::showAndRaise);
    QObject::connect(&tray, &TrayController::nextDhikrRequested,
                     &scheduler, &Scheduler::forceNext);
    QObject::connect(&tray, &TrayController::pauseResumeRequested,
                     &scheduler, &Scheduler::togglePause);

    // ── IPC server: accept commands from other instances / terminal ────────
    QLocalServer::removeServer(IPC_SOCKET);
    QLocalServer ipcServer;
    ipcServer.listen(IPC_SOCKET);
    QObject::connect(&ipcServer, &QLocalServer::newConnection, [&]() {
        QLocalSocket *conn = ipcServer.nextPendingConnection();
        QObject::connect(conn, &QLocalSocket::readyRead, [&, conn]() {
            QString cmd = QString::fromUtf8(conn->readAll()).trimmed();
            if (cmd == "next")         scheduler.forceNext();
            else if (cmd == "settings") settingsWin.showAndRaise();
            else if (cmd == "ping")    conn->write("pong\n");
            else if (cmd.startsWith("mode:")) {
                QString m = cmd.mid(5);
                if (m == "overlay")     settings.setDisplayMode(DisplayMode::OverlayOnly);
                else if (m == "notify") settings.setDisplayMode(DisplayMode::NotificationsOnly);
                else                    settings.setDisplayMode(DisplayMode::Both);
                settings.save();
                overlay.applySettings();
                scheduler.applyNewSettings();
            }
            conn->deleteLater();
        });
    });

    // ── Background update check ────────────────────────────────────────────
    Updater updater("arcalc", "azkare", AZKARE_VERSION);
    QObject::connect(&updater, &Updater::updateAvailable,
                     &tray, &TrayController::showUpdateNotification);
    updater.checkAsync();

    return app.exec();
}
