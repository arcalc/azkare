#include "Updater.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
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
    QNetworkRequest req(
        QString("https://api.github.com/repos/%1/%2/releases/latest")
            .arg(m_owner, m_repo));
    req.setHeader(QNetworkRequest::UserAgentHeader, "Azkare/" + m_currentVersion);
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
    if (!doc.isObject()) { emit checkFailed("Invalid JSON"); return; }

    QJsonObject root = doc.object();
    QString latestTag = root["tag_name"].toString();

    if (isNewer(latestTag, m_currentVersion)) {
        ReleaseInfo info;
        info.tag            = latestTag;
        info.name           = root["name"].toString();
        info.body           = root["body"].toString();
        info.browserUrl     = root["html_url"].toString();
        info.installCommand = terminalUpdateCommand();
        emit updateAvailable(info);
        qInfo() << "[Updater] New version:" << latestTag
                << "— update with:" << info.installCommand;
    } else {
        emit upToDate();
    }
}

// Returns the terminal command a user should run to update
QString Updater::terminalUpdateCommand()
{
#if defined(Q_OS_WIN)
    return "azkare --update";
#else
    return "azkare --update";
#endif
}

bool Updater::isNewer(const QString &a, const QString &b)
{
    auto strip = [](const QString &s) {
        return s.startsWith('v') ? s.mid(1) : s;
    };
    auto parts = [](const QString &s) {
        QList<int> out;
        for (const QString &p : s.split('.')) out << p.toInt();
        while (out.size() < 3) out << 0;
        return out;
    };
    QList<int> va = parts(strip(a)), vb = parts(strip(b));
    for (int i = 0; i < 3; ++i) {
        if (va[i] > vb[i]) return true;
        if (va[i] < vb[i]) return false;
    }
    return false;
}
