#include "AutostartManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

AutostartManager::AutostartManager(QObject *parent) : QObject(parent) {}

QString AutostartManager::executablePath() const
{
    return QCoreApplication::applicationFilePath();
}

bool AutostartManager::isEnabled() const
{
#if defined(Q_OS_LINUX)
    QString desktopFile = QDir::homePath() + "/.config/autostart/azkare.desktop";
    return QFile::exists(desktopFile);
#elif defined(Q_OS_WIN)
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;
    bool found = RegQueryValueExW(key, L"Azkare", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    RegCloseKey(key);
    return found;
#elif defined(Q_OS_MACOS)
    QString plist = QDir::homePath() + "/Library/LaunchAgents/io.github.arcalc.azkare.plist";
    return QFile::exists(plist);
#else
    return false;
#endif
}

bool AutostartManager::setEnabled(bool on)
{
#if defined(Q_OS_LINUX)
    return on ? enableLinux() : disableLinux();
#elif defined(Q_OS_WIN)
    return on ? enableWindows() : disableWindows();
#elif defined(Q_OS_MACOS)
    return on ? enableMacOS() : disableMacOS();
#else
    Q_UNUSED(on)
    return false;
#endif
}

// ── Linux ─────────────────────────────────────────────────────────────────────

bool AutostartManager::enableLinux()
{
    QString dir = QDir::homePath() + "/.config/autostart";
    QDir().mkpath(dir);

    QFile f(dir + "/azkare.desktop");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[AutostartManager] Cannot write .desktop file.";
        return false;
    }

    QTextStream s(&f);
    s << "[Desktop Entry]\n"
      << "Type=Application\n"
      << "Name=Azkare\n"
      << "GenericName=Adhkar Desktop Reminder\n"
      << "Comment=Display Adhkar and Duas on your desktop\n"
      << "Exec=" << executablePath() << "\n"
      << "Icon=azkare\n"
      << "Terminal=false\n"
      << "Categories=Utility;\n"
      << "X-GNOME-Autostart-enabled=true\n";
    return true;
}

bool AutostartManager::disableLinux()
{
    return QFile::remove(QDir::homePath() + "/.config/autostart/azkare.desktop");
}

// ── Windows ───────────────────────────────────────────────────────────────────

bool AutostartManager::enableWindows()
{
#if defined(Q_OS_WIN)
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        qWarning() << "[AutostartManager] Cannot open registry key.";
        return false;
    }
    std::wstring path = executablePath().toStdWString();
    bool ok = RegSetValueExW(key, L"Azkare", 0, REG_SZ,
                             reinterpret_cast<const BYTE *>(path.c_str()),
                             static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)))
              == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
#else
    return false;
#endif
}

bool AutostartManager::disableWindows()
{
#if defined(Q_OS_WIN)
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return false;
    bool ok = RegDeleteValueW(key, L"Azkare") == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
#else
    return false;
#endif
}

// ── macOS ─────────────────────────────────────────────────────────────────────

bool AutostartManager::enableMacOS()
{
    QString dir = QDir::homePath() + "/Library/LaunchAgents";
    QDir().mkpath(dir);

    QFile f(dir + "/io.github.arcalc.azkare.plist");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    QTextStream s(&f);
    s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\"\n"
      << "    \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
      << "<plist version=\"1.0\">\n"
      << "<dict>\n"
      << "    <key>Label</key>\n"
      << "    <string>io.github.arcalc.azkare</string>\n"
      << "    <key>ProgramArguments</key>\n"
      << "    <array>\n"
      << "        <string>" << executablePath() << "</string>\n"
      << "    </array>\n"
      << "    <key>RunAtLoad</key>\n"
      << "    <true/>\n"
      << "    <key>KeepAlive</key>\n"
      << "    <false/>\n"
      << "</dict>\n"
      << "</plist>\n";
    return true;
}

bool AutostartManager::disableMacOS()
{
    return QFile::remove(
        QDir::homePath() + "/Library/LaunchAgents/io.github.arcalc.azkare.plist");
}
