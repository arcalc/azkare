#include "Localizer.h"

#include <QFile>
#include <QJsonDocument>
#include <QDebug>

Localizer::Localizer(QObject *parent) : QObject(parent) {}

void Localizer::flatten(const QString &prefix, const QJsonObject &obj, QHash<QString, QString> &out)
{
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const QString key = prefix.isEmpty() ? it.key() : prefix + QLatin1Char('.') + it.key();
        if (it.value().isObject())
            flatten(key, it.value().toObject(), out);
        else if (it.value().isString())
            out.insert(key, it.value().toString());
    }
}

bool Localizer::readFlat(const QString &path, QHash<QString, QString> &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[Localizer] cannot open" << path;
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Localizer] invalid JSON in" << path << err.errorString();
        return false;
    }
    out.clear();
    flatten(QString(), doc.object(), out);
    return true;
}

bool Localizer::load(const QString &lang)
{
    const QString code = (lang == QLatin1String("en")) ? QStringLiteral("en") : QStringLiteral("ar");
    if (m_fallback.isEmpty())
        readFlat(QStringLiteral(":/i18n/en.json"), m_fallback);
    QHash<QString, QString> next;
    if (!readFlat(QStringLiteral(":/i18n/%1.json").arg(code), next))
        return false;
    const bool changed = (code != m_lang);
    m_cur = next;
    m_lang = code;
    if (changed)
        emit languageChanged();
    return true;
}

QString Localizer::t(const QString &key) const
{
    auto it = m_cur.constFind(key);
    if (it != m_cur.constEnd())
        return it.value();
    it = m_fallback.constFind(key);
    return it != m_fallback.constEnd() ? it.value() : key;
}
