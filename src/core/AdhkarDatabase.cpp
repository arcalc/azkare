#include "AdhkarDatabase.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>

AdhkarDatabase::AdhkarDatabase(QObject *parent) : QObject(parent) {}

bool AdhkarDatabase::load(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[AdhkarDatabase] cannot open" << path;
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[AdhkarDatabase] invalid JSON:" << err.errorString();
        return false;
    }
    m_all.clear();
    m_cats.clear();
    const QJsonArray cats = doc.object().value(QLatin1String("categories")).toArray();
    for (const QJsonValue &cv : cats) {
        const QJsonObject co = cv.toObject();
        AdhkarCategory cat;
        cat.id = co.value(QLatin1String("id")).toString();
        cat.nameAr = co.value(QLatin1String("name_ar")).toString();
        cat.nameEn = co.value(QLatin1String("name_en")).toString();
        if (cat.id.isEmpty())
            continue;
        for (const QJsonValue &ev : co.value(QLatin1String("entries")).toArray()) {
            const QJsonObject eo = ev.toObject();
            AdhkarEntry e;
            e.id = eo.value(QLatin1String("id")).toString();
            e.categoryId = cat.id;
            e.textAr = eo.value(QLatin1String("text_ar")).toString();
            if (e.id.isEmpty() || e.textAr.isEmpty())
                continue;
            e.translationEn = eo.value(QLatin1String("translation_en")).toString();
            e.repeat = qMax(1, eo.value(QLatin1String("count")).toInt(1));
            e.sourceAr = eo.value(QLatin1String("source_ar")).toString();
            e.sourceEn = eo.value(QLatin1String("source_en")).toString();
            m_all.append(e);
            ++cat.count;
        }
        m_cats.append(cat);
    }
    m_cursor = 0;
    rebuildActiveList();
    qInfo() << "[AdhkarDatabase] loaded" << m_all.size() << "entries in" << m_cats.size() << "categories";
    return !m_all.isEmpty();
}

QString AdhkarDatabase::categoryName(const QString &id, const QString &lang) const
{
    for (const AdhkarCategory &c : m_cats)
        if (c.id == id)
            return lang == QLatin1String("en") ? c.nameEn : c.nameAr;
    return id;
}

void AdhkarDatabase::setActiveCategories(const QStringList &ids)
{
    m_activeCategories = ids;
    m_cursor = 0;
    rebuildActiveList();
}

void AdhkarDatabase::setShuffleEnabled(bool on)
{
    m_shuffle = on;
    m_cursor = 0;
    rebuildActiveList();
}

const AdhkarEntry &AdhkarDatabase::current() const
{
    static const AdhkarEntry empty;
    return m_active.isEmpty() ? empty : m_all.at(m_active.at(qBound(0, m_cursor, m_active.size() - 1)));
}

const AdhkarEntry &AdhkarDatabase::next()
{
    if (m_active.isEmpty())
        return current();
    m_cursor = (m_cursor + 1) % m_active.size();
    if (m_cursor == 0 && m_shuffle)
        shuffleList();
    return current();
}

void AdhkarDatabase::reset()
{
    m_cursor = 0;
    rebuildActiveList();
}

void AdhkarDatabase::rebuildActiveList()
{
    m_active.clear();
    if (!m_activeCategories.isEmpty()) {
        for (int i = 0; i < m_all.size(); ++i)
            if (m_activeCategories.contains(m_all.at(i).categoryId))
                m_active.append(i);
    }
    if (m_active.isEmpty()) { // empty / stale selection -> everything
        for (int i = 0; i < m_all.size(); ++i)
            m_active.append(i);
    }
    if (m_shuffle)
        shuffleList();
}

void AdhkarDatabase::shuffleList()
{
    for (int i = m_active.size() - 1; i > 0; --i) {
        const int j = static_cast<int>(QRandomGenerator::global()->bounded(i + 1));
        std::swap(m_active[i], m_active[j]);
    }
}
