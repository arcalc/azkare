#include "Updater.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Updater::Updater(const QString &owner,
                 const QString &repo,
                 const QString &currentVersion,
                 QObject       *parent)
    : QObject(parent)
    , m_owner(owner)
    , m_repo(repo)
    , m_currentVersion(currentVersion)
{
    connect(&m_nam, &QNetworkAccessManager::finished,
            this,   &Updater::onReplyFinished);
}

void Updater::checkAsync()
{
    QString url = QString("https://api.github.com/repos/%1/%2/releases/latest")
                      .arg(m_owner, m_repo);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Azkare/" + m_currentVersion);
    req.setRawHeader("Accept", "application/vnd.github+json");
    m_nam.get(req);
}

void Updater::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit checkFailed("Invalid JSON from GitHub API");
        return;
    }

    QJsonObject root = doc.object();
    QString latestTag  = root["tag_name"].toString();   // e.g. "v1.2.0"
    QString releaseName = root["name"].toString();
    QString releaseBody = root["body"].toString();
    QString htmlUrl     = root["html_url"].toString();

    // Find the asset for the current platform
    QString downloadUrl = htmlUrl; // fallback
    QString platform;
#if defined(Q_OS_LINUX)
    platform = "linux";
#elif defined(Q_OS_WIN)
    platform = "windows";
#elif defined(Q_OS_MACOS)
    platform = "macos";
#endif

    QJsonArray assets = root["assets"].toArray();
    for (const QJsonValue &v : assets) {
        QJsonObject asset = v.toObject();
        QString assetName = asset["name"].toString().toLower();
        if (assetName.contains(platform)) {
            downloadUrl = asset["browser_download_url"].toString();
            break;
        }
    }

    if (isNewer(latestTag, m_currentVersion)) {
        ReleaseInfo info;
        info.tag         = latestTag;
        info.name        = releaseName;
        info.body        = releaseBody;
        info.browserUrl  = htmlUrl;
        info.downloadUrl = downloadUrl;
        emit updateAvailable(info);
        qInfo() << "[Updater] New version available:" << latestTag;
    } else {
        emit upToDate();
        qInfo() << "[Updater] Up to date.";
    }
}

bool Updater::isNewer(const QString &a, const QString &b)
{
    // Strip leading 'v'
    auto strip = [](const QString &s) -> QString {
        return s.startsWith('v') ? s.mid(1) : s;
    };

    auto parts = [](const QString &s) -> QList<int> {
        QList<int> out;
        for (const QString &p : s.split('.'))
            out << p.toInt();
        while (out.size() < 3) out << 0;
        return out;
    };

    QList<int> va = parts(strip(a));
    QList<int> vb = parts(strip(b));

    for (int i = 0; i < 3; ++i) {
        if (va[i] > vb[i]) return true;
        if (va[i] < vb[i]) return false;
    }
    return false;
}
