#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class QNetworkReply;

struct ReleaseInfo {
    QString tag;            ///< "v1.2.3"
    QString name;
    QString body;           ///< release notes (Markdown)
    QString browserUrl;     ///< GitHub release page
    QString installCommand; ///< terminal command that performs the update
};

/**
 * Checks GitHub for the newest release (falls back to the newest v* tag when no
 * release exists). Never downloads binaries: updating is done by `azkare --update`.
 */
class Updater : public QObject
{
    Q_OBJECT
public:
    Updater(const QString &owner, const QString &repo, const QString &currentVersion, QObject *parent = nullptr);

    void checkAsync();
    static bool isNewer(const QString &a, const QString &b); ///< semantic "a > b", leading 'v' ignored

signals:
    void latestKnown(const QString &tag);  ///< newest tag found (before the newer/up-to-date decision)
    void updateAvailable(const ReleaseInfo &info);
    void upToDate();
    void checkFailed(const QString &reason);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    void request(const QString &path);
    void finishWith(const ReleaseInfo &info);

    QNetworkAccessManager m_nam;
    QString m_owner, m_repo, m_current;
    bool m_tagsPhase = false;
};
