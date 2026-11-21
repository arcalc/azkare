#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct ReleaseInfo {
    QString tag;            ///< e.g. "v1.1.0"
    QString name;           ///< Release title
    QString body;           ///< Markdown release notes
    QString browserUrl;     ///< GitHub release page URL
    QString installCommand; ///< Terminal command to update (install.sh/ps1)
};

/**
 * Checks https://api.github.com/repos/<owner>/<repo>/releases/latest
 * and emits updateAvailable() with a terminal install command (not a binary URL).
 */
class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(const QString &owner,
                     const QString &repo,
                     const QString &currentVersion,
                     QObject       *parent = nullptr);

    void checkAsync();

signals:
    void updateAvailable(const ReleaseInfo &info);
    void upToDate();
    void checkFailed(const QString &reason);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    static bool isNewer(const QString &a, const QString &b);
    static QString terminalUpdateCommand();

    QNetworkAccessManager m_nam;
    QString m_owner;
    QString m_repo;
    QString m_currentVersion;
};
