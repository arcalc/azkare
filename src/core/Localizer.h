#pragma once
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>

/** Loads :/i18n/<lang>.json (nested keys are flattened to "a.b") and serves translations. */
class Localizer : public QObject
{
    Q_OBJECT
public:
    explicit Localizer(QObject *parent = nullptr);

    /** Load "ar" or "en" (anything else becomes "ar"). Emits languageChanged() on change. */
    bool load(const QString &lang);
    QString language() const { return m_lang; }
    bool isRtl() const { return m_lang == QLatin1String("ar"); }
    Qt::LayoutDirection layoutDirection() const { return isRtl() ? Qt::RightToLeft : Qt::LeftToRight; }

    /** Translation for key; falls back to English, then to the key itself. */
    QString t(const QString &key) const;

signals:
    void languageChanged();

private:
    static bool readFlat(const QString &path, QHash<QString, QString> &out);
    static void flatten(const QString &prefix, const QJsonObject &obj, QHash<QString, QString> &out);

    QString m_lang;
    QHash<QString, QString> m_cur;
    QHash<QString, QString> m_fallback;
};
