#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct ReleaseInfo {
    QString tag;         ///< e.g. "v1.1.0"
    QString name;        ///< Release title
    QString body;        ///< Markdown release notes
    QString browserUrl;  ///< https://github.com/arcalc/azkare/releases/tag/v1.1.0
    QString downloadUrl; ///< Direct asset URL for current platform
};

/**
 * Queries https://api.github.com/repos/<owner>/<repo>/releases/latest
 * and emits updateAvailable() when a newer version exists.
 */
class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(const QString &owner,
                     const QString &repo,
                     const QString &currentVersion,
                     QObject       *parent = nullptr);

    /** Kick off a background check — non-blocking. */
    void checkAsync();

signals:
    void updateAvailable(const ReleaseInfo &info);
    void upToDate();
    void checkFailed(const QString &reason);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    /** Simple semver comparison: returns true if a > b */
    static bool isNewer(const QString &a, const QString &b);

    QNetworkAccessManager m_nam;
    QString m_owner;
    QString m_repo;
    QString m_currentVersion;
};
