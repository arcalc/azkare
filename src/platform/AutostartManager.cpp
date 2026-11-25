#include "AutostartManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QDebug>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

static QString exePath() { return QCoreApplication::applicationFilePath(); }

QString AutostartManager::entryPath()
{
#if defined(Q_OS_MACOS)
    return QDir::homePath() + QStringLiteral("/Library/LaunchAgents/io.github.arcalc.azkare.plist");
#elif defined(Q_OS_WIN)
    return QString();
#else
    return QDir::homePath() + QStringLiteral("/.config/autostart/azkare.desktop");
#endif
}

bool AutostartManager::isEnabled() const
{
#if defined(Q_OS_WIN)
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;
    const bool found = RegQueryValueExW(key, L"Azkare", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    RegCloseKey(key);
    return found;
#else
    return QFile::exists(entryPath());
#endif
}

bool AutostartManager::setEnabled(bool on)
{
#if defined(Q_OS_WIN)
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return false;
    bool ok;
    if (on) {
        const std::wstring cmd = (QStringLiteral("\"") + QDir::toNativeSeparators(exePath()) + QStringLiteral("\" --foreground")).toStdWString();
        ok = RegSetValueExW(key, L"Azkare", 0, REG_SZ, reinterpret_cast<const BYTE *>(cmd.c_str()),
                            static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    } else {
        const LONG r = RegDeleteValueW(key, L"Azkare");
        ok = (r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND);
    }
    RegCloseKey(key);
    return ok;
#else
    const QString path = entryPath();
    if (!on)
        return !QFile::exists(path) || QFile::remove(path);

    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[Autostart] cannot write" << path;
        return false;
    }
    QTextStream s(&f);
#  if defined(Q_OS_MACOS)
    QString esc = exePath().toHtmlEscaped();
    s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
         "<plist version=\"1.0\"><dict>\n"
         "  <key>Label</key><string>io.github.arcalc.azkare</string>\n"
         "  <key>ProgramArguments</key><array><string>" << esc << "</string><string>--foreground</string></array>\n"
         "  <key>RunAtLoad</key><true/>\n"
         "</dict></plist>\n";
#  else
    s << "[Desktop Entry]\nType=Application\nName=Azkare\nComment=Adhkar and Duas on your desktop\n"
         "Exec=\"" << exePath() << "\" --foreground\nIcon=azkare\nTerminal=false\n"
         "X-GNOME-Autostart-enabled=true\n";
#  endif
    s.flush();
    return f.commit();
#endif
}
