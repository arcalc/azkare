#include "Updater.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

Updater::Updater(const QString &owner, const QString &repo, const QString &currentVersion, QObject *parent)
    : QObject(parent), m_owner(owner), m_repo(repo), m_current(currentVersion)
{
    connect(&m_nam, &QNetworkAccessManager::finished, this, &Updater::onReplyFinished);
}

void Updater::request(const QString &path)
{
    QNetworkRequest req(QUrl(QStringLiteral("https://api.github.com/repos/%1/%2/%3").arg(m_owner, m_repo, path)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Azkare/") + m_current);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setTransferTimeout(10000);
    m_nam.get(req);
}

void Updater::checkAsync()
{
    m_tagsPhase = false;
    request(QStringLiteral("releases/latest"));
}

void Updater::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (!m_tagsPhase && status == 404) { // repo has no published release yet -> look at tags
        m_tagsPhase = true;
        request(QStringLiteral("tags?per_page=100"));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed(reply->errorString());
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());

    ReleaseInfo info;
    info.installCommand = QStringLiteral("azkare --update");
    if (!m_tagsPhase) {
        if (!doc.isObject()) { emit checkFailed(QStringLiteral("unexpected response from GitHub")); return; }
        const QJsonObject o = doc.object();
        info.tag = o.value(QLatin1String("tag_name")).toString();
        info.name = o.value(QLatin1String("name")).toString();
        info.body = o.value(QLatin1String("body")).toString();
        info.browserUrl = o.value(QLatin1String("html_url")).toString();
    } else {
        static const QRegularExpression semver(QStringLiteral("^v?\\d+\\.\\d+\\.\\d+$"));
        for (const QJsonValue &v : doc.array()) {
            const QString name = v.toObject().value(QLatin1String("name")).toString();
            if (semver.match(name).hasMatch() && (info.tag.isEmpty() || isNewer(name, info.tag)))
                info.tag = name;
        }
        info.name = info.tag;
        info.browserUrl = QStringLiteral("https://github.com/%1/%2/releases/tag/%3").arg(m_owner, m_repo, info.tag);
    }
    if (info.tag.isEmpty()) { // nothing published yet: we are the newest
        emit upToDate();
        return;
    }
    finishWith(info);
}

void Updater::finishWith(const ReleaseInfo &info)
{
    emit latestKnown(info.tag);
    if (isNewer(info.tag, m_current))
        emit updateAvailable(info);
    else
        emit upToDate();
}

bool Updater::isNewer(const QString &a, const QString &b)
{
    auto parts = [](QString s) {
        if (s.startsWith(QLatin1Char('v'))) s.remove(0, 1);
        QList<int> out;
        for (const QString &p : s.split(QLatin1Char('.'))) out << p.toInt();
        while (out.size() < 3) out << 0;
        return out;
    };
    const QList<int> va = parts(a), vb = parts(b);
    for (int i = 0; i < 3; ++i) {
        if (va[i] != vb[i]) return va[i] > vb[i];
    }
    return false;
}
