/**
 * Azkare (أذكاري) - Adhkar & Duas on your desktop.
 *
 * Process roles (decided from the command line BEFORE any GUI object exists, so that
 * --help / --version / --status also work over SSH without a display):
 *   CLI       --help --version --status --next --mode --autostart --update --uninstall --settings
 *   settings  --settings-window   (internal: the settings UI, a separate process)
 *   daemon    (default) overlay + tray + scheduler; detaches from the terminal unless --foreground
 */
#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <cstdio>

#include "Version.h"
#include "core/AdhkarDatabase.h"
#include "core/AppSettings.h"
#include "core/Ipc.h"
#include "core/Localizer.h"
#include "core/Scheduler.h"
#include "core/Updater.h"
#include "notifications/NotificationDispatcher.h"
#include "overlay/OverlayManager.h"
#include "platform/AutostartManager.h"
#include "platform/TrayController.h"
#include "ui/AppIcon.h"
#include "ui/SettingsWindow.h"

#if defined(Q_OS_WIN)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace {

const char *kRawBase = "https://raw.githubusercontent.com/arcalc/azkare/main";

void say(const QString &s) { std::fputs(s.toUtf8().constData(), stdout); std::fputc('\n', stdout); std::fflush(stdout); }
void warnLine(const QString &s) { std::fputs(s.toUtf8().constData(), stderr); std::fputc('\n', stderr); }

void addOptions(QCommandLineParser &p)
{
    p.addOptions({
        {{QStringLiteral("v"), QStringLiteral("version")}, QStringLiteral("Print version")},
        {{QStringLiteral("h"), QStringLiteral("help")}, QStringLiteral("Print help")},
        {QStringLiteral("start"), QStringLiteral("Start overlay + tray (default)")},
        {QStringLiteral("foreground"), QStringLiteral("Do not detach from the terminal")},
        {QStringLiteral("settings"), QStringLiteral("Open the settings window")},
        {QStringLiteral("settings-window"), QStringLiteral("(internal) run the settings UI in this process")},
        {QStringLiteral("next"), QStringLiteral("Show the next Dhikr now")},
        {QStringLiteral("quit"), QStringLiteral("Stop the background instance")},
        {QStringLiteral("status"), QStringLiteral("Print status")},
        {QStringLiteral("update"), QStringLiteral("Update from GitHub and rebuild")},
        {QStringLiteral("force"), QStringLiteral("With --update: rebuild even if already up to date")},
        {QStringLiteral("uninstall"), QStringLiteral("Remove Azkare")},
        {QStringLiteral("purge"), QStringLiteral("With --uninstall: also delete config and source")},
        {QStringLiteral("mode"), QStringLiteral("Display mode"), QStringLiteral("overlay|notify|both")},
        {QStringLiteral("autostart"), QStringLiteral("Login autostart"), QStringLiteral("on|off")},
    });
}

void setupApp(QCoreApplication &a)
{
    a.setApplicationName(QStringLiteral("Azkare"));   // no organization name: config lives in <config>/Azkare
    a.setApplicationVersion(QStringLiteral(AZKARE_VERSION));
    a.setOrganizationDomain(QStringLiteral("io.github.arcalc.azkare"));
}

void printHelp()
{
    say(QString::fromUtf8(
"Azkare " AZKARE_VERSION " — أذكاري\n"
"Adhkar & Duas on your desktop  |  أذكار وأدعية على سطح مكتبك\n"
"\n"
"Usage / الاستخدام:  azkare [option]\n"
"\n"
"  (none), --start          Start overlay + tray in the background | تشغيل أذكاري في الخلفية\n"
"  --foreground             Stay attached to this terminal | البقاء في الطرفية\n"
"  --settings               Open the settings window | فتح نافذة الإعدادات\n"
"  --next                   Show the next Dhikr now | الذكر التالي فورًا\n"
"  --quit                   Stop the background instance | إيقاف أذكاري\n"
"  --mode overlay|notify|both\n"
"                           Display mode | وضع العرض (سطح المكتب / إشعارات / الاثنين)\n"
"  --autostart on|off       Start at login | التشغيل عند بدء النظام\n"
"  --update [--force]       Check GitHub, pull, rebuild, restart | التحديث من المصدر\n"
"  --uninstall [--purge]    Remove Azkare (--purge: also config + source) | إلغاء التثبيت\n"
"  --status                 Show whether it is running | الحالة\n"
"  --version, -v            Print version | الإصدار\n"
"  --help, -h               This help | المساعدة\n"
"\n"
"Project: https://github.com/arcalc/azkare"));
}

bool spawnDetached(const QStringList &args)
{
    QProcess p;
    p.setProgram(QCoreApplication::applicationFilePath());
    p.setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove(QStringLiteral("QT_WAYLAND_SHELL_INTEGRATION")); // the daemon may have set it; UI windows must not inherit it
    p.setProcessEnvironment(env);
#if defined(Q_OS_WIN)
    p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a) {
        a->flags |= CREATE_NO_WINDOW;
        a->flags &= ~DWORD(CREATE_NEW_CONSOLE);
    });
#endif
    return p.startDetached();
}

/** Runs install.sh / install.ps1 (local copy if present, else straight from GitHub). */
int runInstaller(const QString &action, const QStringList &extra, bool restart = false)
{
#if defined(Q_OS_WIN)
    // azkare.exe cannot overwrite itself while running: let a separate PowerShell finish the job after we exit.
    const QString local = qEnvironmentVariable("LOCALAPPDATA") + QStringLiteral("/Azkare/src/install.ps1");
    QString flags = action == QLatin1String("--update") ? QStringLiteral("-Update") : QStringLiteral("-Uninstall");
    if (extra.contains(QLatin1String("--purge"))) flags += QStringLiteral(" -Purge");
    if (restart) flags += QStringLiteral(" -Restart");
    const QString cmd = QFile::exists(local)
        ? QStringLiteral("& '%1' %2").arg(QDir::toNativeSeparators(local), flags)
        : QStringLiteral("& ([scriptblock]::Create((irm %1/install.ps1))) %2").arg(QLatin1String(kRawBase), flags);
    const bool ok = QProcess::startDetached(QStringLiteral("powershell"),
        {QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), cmd});
    say(ok ? QStringLiteral("The installer continues in a separate PowerShell window.") : QStringLiteral("Could not start PowerShell."));
    return ok ? 0 : 1;
#else
    QProcess pr;
    pr.setProcessChannelMode(QProcess::ForwardedChannels);
    pr.setInputChannelMode(QProcess::ForwardedInputChannel);
    const QString local = QDir::homePath() + QStringLiteral("/.local/share/azkare-src/install.sh");
    pr.setProgram(QStringLiteral("/bin/bash"));
    if (QFile::exists(local))
        pr.setArguments(QStringList{local, action} + extra);
    else
        pr.setArguments({QStringLiteral("-c"), QStringLiteral("curl -fsSL %1/install.sh | bash -s -- %2 %3")
                                                    .arg(QLatin1String(kRawBase), action, extra.join(QLatin1Char(' ')))});
    pr.start();
    if (!pr.waitForStarted(5000)) {
        warnLine(QStringLiteral("Could not start the installer script."));
        return 1;
    }
    pr.waitForFinished(-1);
    return pr.exitStatus() == QProcess::NormalExit ? pr.exitCode() : 1;
#endif
}

int runUpdate(bool force)
{
    say(QStringLiteral("[أذكاري] Current version: v%1").arg(QLatin1String(AZKARE_VERSION)));
    Updater up(QStringLiteral("arcalc"), QStringLiteral("azkare"), QStringLiteral(AZKARE_VERSION));
    QEventLoop loop;
    QString latest, failure;
    bool newer = false;
    QObject::connect(&up, &Updater::latestKnown, [&](const QString &t) { latest = t; });
    QObject::connect(&up, &Updater::updateAvailable, [&](const ReleaseInfo &i) {
        newer = true;
        if (!i.body.trimmed().isEmpty()) say(QStringLiteral("\nRelease notes (%1):\n%2\n").arg(i.tag, i.body.trimmed()));
        loop.quit();
    });
    QObject::connect(&up, &Updater::upToDate, &loop, &QEventLoop::quit);
    QObject::connect(&up, &Updater::checkFailed, [&](const QString &w) { failure = w; loop.quit(); });
    QTimer::singleShot(12000, &loop, &QEventLoop::quit);
    up.checkAsync();
    loop.exec();

    if (!failure.isEmpty())
        say(QStringLiteral("[!] Could not query GitHub (%1) - continuing with the latest source on main.").arg(failure));
    else if (!latest.isEmpty())
        say(QStringLiteral("[أذكاري] Latest release: %1").arg(latest));
    if (failure.isEmpty() && !latest.isEmpty() && !newer && !force) {
        say(QStringLiteral("[✓] Already up to date. Use --update --force to rebuild from main anyway."));
        return 0;
    }

    const bool wasRunning = Ipc::send(Ipc::daemonName(), QStringLiteral("ping"));
    const int rc = runInstaller(QStringLiteral("--update"), {}, wasRunning);
    if (rc != 0) {
        warnLine(QStringLiteral("Update failed (exit %1). The installed version was left untouched.").arg(rc));
        return rc;
    }
#if !defined(Q_OS_WIN)
    if (wasRunning) {
        say(QStringLiteral("[أذكاري] Restarting Azkare..."));
        Ipc::send(Ipc::daemonName(), QStringLiteral("quit"));
        QThread::msleep(800);
        spawnDetached({QStringLiteral("--foreground")});
    }
#endif
    return 0;
}

int runUninstall(bool purge)
{
    if (Ipc::send(Ipc::daemonName(), QStringLiteral("quit")))
        QThread::msleep(500);
    return runInstaller(QStringLiteral("--uninstall"), purge ? QStringList{QStringLiteral("--purge")} : QStringList{});
}

int runCli(QCommandLineParser &p)
{
    if (p.isSet(QStringLiteral("help"))) { printHelp(); return 0; }
    if (p.isSet(QStringLiteral("version"))) { say(QString::fromLatin1(AZKARE_VERSION_FULL)); return 0; }
    if (p.isSet(QStringLiteral("update"))) return runUpdate(p.isSet(QStringLiteral("force")));
    if (p.isSet(QStringLiteral("uninstall"))) return runUninstall(p.isSet(QStringLiteral("purge")));

    if (p.isSet(QStringLiteral("autostart"))) {
        const QString v = p.value(QStringLiteral("autostart")).toLower();
        if (v != QLatin1String("on") && v != QLatin1String("off")) { warnLine(QStringLiteral("--autostart needs 'on' or 'off'")); return 2; }
        const bool ok = AutostartManager().setEnabled(v == QLatin1String("on"));
        say(ok ? QStringLiteral("Autostart %1").arg(v) : QStringLiteral("Could not change autostart (check permissions)"));
        return ok ? 0 : 1;
    }
    if (p.isSet(QStringLiteral("quit"))) {
        if (!Ipc::send(Ipc::daemonName(), QStringLiteral("quit"))) { say(QStringLiteral("Azkare is not running.")); return 0; }
        say(QStringLiteral("Azkare stopped."));
        return 0;
    }
    if (p.isSet(QStringLiteral("next"))) {
        if (!Ipc::send(Ipc::daemonName(), QStringLiteral("next"))) { warnLine(QStringLiteral("Azkare is not running. Start it with: azkare")); return 1; }
        return 0;
    }
    if (p.isSet(QStringLiteral("mode"))) {
        const QString m = p.value(QStringLiteral("mode")).toLower();
        if (m != QLatin1String("overlay") && m != QLatin1String("notify") && m != QLatin1String("both")) {
            warnLine(QStringLiteral("--mode needs overlay, notify or both")); return 2;
        }
        if (Ipc::send(Ipc::daemonName(), QStringLiteral("mode:") + m)) { say(QStringLiteral("Mode: %1").arg(m)); return 0; }
        AppSettings s;
        s.load();
        s.setDisplayMode(m == QLatin1String("overlay") ? DisplayMode::OverlayOnly : m == QLatin1String("notify") ? DisplayMode::NotificationsOnly : DisplayMode::Both);
        s.save();
        say(QStringLiteral("Mode: %1 (Azkare is not running; applies at next start)").arg(m));
        return 0;
    }
    if (p.isSet(QStringLiteral("status"))) {
        QString reply;
        const bool up = Ipc::send(Ipc::daemonName(), QStringLiteral("ping"), &reply);
        say(QString::fromLatin1(AZKARE_VERSION_FULL));
        say(QStringLiteral("Status: ") + (up ? QStringLiteral("running") : QStringLiteral("not running")));
        say(QStringLiteral("Config: ") + AppSettings::configFilePath());
        say(QStringLiteral("Autostart: ") + (AutostartManager().isEnabled() ? QStringLiteral("on") : QStringLiteral("off")));
        return 0;
    }
    if (p.isSet(QStringLiteral("settings"))) {
        if (Ipc::send(Ipc::settingsName(), QStringLiteral("raise")))
            return 0; // already open: just brought to front
        return spawnDetached({QStringLiteral("--settings-window")}) ? 0 : 1;
    }
    return 0;
}

QStringList registerBundledFonts()
{
    QStringList families;
    const QStringList files = QDir(QStringLiteral(":/fonts")).entryList({QStringLiteral("*.ttf"), QStringLiteral("*.otf")});
    for (const QString &f : files) {
        const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/") + f);
        if (id < 0) qWarning() << "[Azkare] could not load font" << f;
        else families << QFontDatabase::applicationFontFamilies(id);
    }
    return families;
}

int runSettingsWindow(int argc, char **argv)
{
    QApplication app(argc, argv);
    setupApp(app);
    app.setQuitOnLastWindowClosed(true);
    app.setWindowIcon(appIcon());
    QGuiApplication::setDesktopFileName(QStringLiteral("azkare"));
    registerBundledFonts();

    AppSettings settings;
    settings.load();
    Localizer loc;
    loc.load(settings.language());
    QApplication::setLayoutDirection(loc.layoutDirection());
    AdhkarDatabase db;
    if (!db.load(QStringLiteral(":/data/adhkar.json"))) { warnLine(QStringLiteral("Cannot load the Adhkar dataset")); return 1; }
    Updater updater(QStringLiteral("arcalc"), QStringLiteral("azkare"), QStringLiteral(AZKARE_VERSION));

    SettingsWindow win(settings, db, loc, updater);

    QLocalServer::removeServer(Ipc::settingsName());
    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    server.listen(Ipc::settingsName());
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        if (QLocalSocket *c = server.nextPendingConnection()) {
            c->readAll();
            c->disconnectFromServer();
            c->deleteLater();
            win.showAndRaise();
        }
    });
    win.showAndRaise();
    return app.exec();
}

int runDaemon(int argc, char **argv)
{
#if defined(AZKARE_HAS_LAYERSHELL)
    // Wayland: draw the overlay as a layer-shell surface (only this daemon process, never a settings window)
    if (qEnvironmentVariableIsEmpty("QT_WAYLAND_SHELL_INTEGRATION") && qgetenv("QT_QPA_PLATFORM") != "xcb"
        && (qgetenv("XDG_SESSION_TYPE") == "wayland" || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")))
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
#endif
    QApplication app(argc, argv);
    setupApp(app);
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(appIcon());
    QGuiApplication::setDesktopFileName(QStringLiteral("azkare"));
    registerBundledFonts();

    AppSettings settings;
    settings.load();
    Localizer loc;
    loc.load(settings.language());
    QApplication::setLayoutDirection(loc.layoutDirection());

    AdhkarDatabase db;
    if (!db.load(QStringLiteral(":/data/adhkar.json"))) { warnLine(QStringLiteral("Fatal: cannot load the Adhkar dataset")); return 1; }
    db.setActiveCategories(settings.activeCategories());
    db.setShuffleEnabled(settings.shuffleEnabled());

    OverlayManager overlay(settings);
    NotificationDispatcher notifier(settings);
    Scheduler scheduler(settings, db, overlay, notifier);
    TrayController tray(loc, scheduler);
    notifier.setTrayIcon(tray.icon());
    tray.show();
    overlay.initialize();
    scheduler.start();

    auto applyAll = [&]() {
        loc.load(settings.language());
        QApplication::setLayoutDirection(loc.layoutDirection());
        db.setActiveCategories(settings.activeCategories());
        db.setShuffleEnabled(settings.shuffleEnabled());
        overlay.applySettings();
        scheduler.applyNewSettings();
    };
    auto reload = [&]() { settings.resetToDefaults(); settings.load(); applyAll(); };

    QObject::connect(&tray, &TrayController::nextDhikrRequested, &scheduler, &Scheduler::forceNext);
    QObject::connect(&tray, &TrayController::pauseResumeRequested, &scheduler, &Scheduler::togglePause);
    QObject::connect(&tray, &TrayController::openSettingsRequested, [&]() {
        if (!Ipc::send(Ipc::settingsName(), QStringLiteral("raise")))
            spawnDetached({QStringLiteral("--settings-window")});
    });

    Updater updater(QStringLiteral("arcalc"), QStringLiteral("azkare"), QStringLiteral(AZKARE_VERSION));
    bool manualCheck = false;
    QObject::connect(&updater, &Updater::updateAvailable, &tray, &TrayController::showUpdateAvailable);
    QObject::connect(&updater, &Updater::upToDate, [&]() {
        if (manualCheck) tray.showMessage(loc.t(QStringLiteral("app.name")), loc.t(QStringLiteral("settings.about_up_to_date")));
        manualCheck = false;
    });
    QObject::connect(&updater, &Updater::checkFailed, [&](const QString &why) {
        if (manualCheck) tray.showMessage(loc.t(QStringLiteral("app.name")), loc.t(QStringLiteral("settings.about_check_failed")) + QStringLiteral(" ") + why);
        manualCheck = false;
    });
    QObject::connect(&tray, &TrayController::checkUpdatesRequested, [&]() { manualCheck = true; updater.checkAsync(); });
    updater.checkAsync(); // silent: only speaks up when a newer version exists

    // ── IPC server ───────────────────────────────────────────────────────────
    QLocalServer::removeServer(Ipc::daemonName());
    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    if (!server.listen(Ipc::daemonName()))
        qWarning() << "[Azkare] IPC unavailable:" << server.errorString();
    bool dragMoved = false;
    QObject::connect(&overlay, &OverlayManager::overlayMoved, [&]() { dragMoved = true; });
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *c = server.nextPendingConnection();
        if (!c) return;
        QObject::connect(c, &QLocalSocket::disconnected, c, &QObject::deleteLater);
        QObject::connect(c, &QLocalSocket::readyRead, [&, c]() {
            const QString cmd = QString::fromUtf8(c->readAll()).trimmed();
            QString reply = QStringLiteral("ok");
            if (cmd == QLatin1String("next")) scheduler.forceNext();
            else if (cmd == QLatin1String("reload")) reload();
            else if (cmd == QLatin1String("ping")) reply = QStringLiteral("pong ") + QLatin1String(AZKARE_VERSION);
            else if (cmd == QLatin1String("quit")) QTimer::singleShot(0, &app, &QCoreApplication::quit);
            else if (cmd == QLatin1String("settings")) { if (!Ipc::send(Ipc::settingsName(), QStringLiteral("raise"))) spawnDetached({QStringLiteral("--settings-window")}); }
            else if (cmd == QLatin1String("drag:on")) { dragMoved = false; overlay.setDragMode(true); }
            else if (cmd == QLatin1String("drag:off")) {
                overlay.setDragMode(false);
                if (dragMoved) { const QPoint p = settings.overlayCustomPos(); reply = QStringLiteral("custom %1 %2").arg(p.x()).arg(p.y()); }
                dragMoved = false;
            } else if (cmd.startsWith(QLatin1String("mode:"))) {
                const QString m = cmd.mid(5);
                settings.setDisplayMode(m == QLatin1String("overlay") ? DisplayMode::OverlayOnly : m == QLatin1String("notify") ? DisplayMode::NotificationsOnly : DisplayMode::Both);
                settings.save();
                applyAll();
            } else reply = QStringLiteral("unknown");
            c->write((reply + QLatin1Char('\n')).toUtf8());
            c->flush();
            c->disconnectFromServer();
        });
    });

    return app.exec();
}

} // namespace

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    SetConsoleOutputCP(CP_UTF8);
#endif
    // Decide the process role from raw arguments, before creating any application object.
    static const QStringList cliFlags{"-v", "--version", "-h", "--help", "--status", "--next", "--quit", "--mode", "--autostart",
                                      "--update", "--uninstall", "--settings"};
    bool cli = false, settingsWindow = false, foreground = false;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]).section(QLatin1Char('='), 0, 0);
        if (cliFlags.contains(a)) cli = true;
        if (a == QLatin1String("--settings-window")) settingsWindow = true;
        if (a == QLatin1String("--foreground")) foreground = true;
    }

    if (settingsWindow)
        return runSettingsWindow(argc, argv);

    if (cli) {
        QCoreApplication app(argc, argv);
        setupApp(app);
        QCommandLineParser parser;
        addOptions(parser);
        parser.process(app);
        return runCli(parser);
    }

    // Daemon. Without --foreground, hand over to a detached child and return to the prompt.
    if (!foreground) {
        QCoreApplication probe(argc, argv);
        setupApp(probe);
        QCommandLineParser parser;
        addOptions(parser);
        parser.process(probe);
        if (Ipc::send(Ipc::daemonName(), QStringLiteral("ping"))) {
            say(QStringLiteral("Azkare is already running."));
            return 0;
        }
        if (spawnDetached({QStringLiteral("--foreground")})) {
            say(QStringLiteral("Azkare started in the background. Try: azkare --status | azkare --help"));
            return 0;
        }
        warnLine(QStringLiteral("Could not start in the background; running in this terminal."));
    }
    return runDaemon(argc, argv);
}
