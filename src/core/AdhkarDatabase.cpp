#include "AdhkarDatabase.h"

#include <QFile>
#include <QRandomGenerator>
#include <QDebug>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

AdhkarDatabase::AdhkarDatabase(QObject *parent)
    : QObject(parent)
{}

bool AdhkarDatabase::load(const QString &jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[AdhkarDatabase] Cannot open:" << jsonPath;
        return false;
    }

    QByteArray raw = file.readAll();
    file.close();

    try {
        json root = json::parse(raw.constData());

        m_all.clear();

        for (const auto &cat : root.at("categories")) {
            QString catId = QString::fromStdString(cat.at("id").get<std::string>());

            for (const auto &e : cat.at("entries")) {
                AdhkarEntry entry;
                entry.id             = QString::fromStdString(e.at("id").get<std::string>());
                entry.categoryId     = catId;
                entry.textAr         = QString::fromStdString(e.at("text_ar").get<std::string>());

                if (e.contains("transliteration"))
                    entry.transliteration = QString::fromStdString(e.at("transliteration").get<std::string>());
                if (e.contains("translation_en"))
                    entry.translationEn = QString::fromStdString(e.at("translation_en").get<std::string>());
                if (e.contains("repeat"))
                    entry.repeat = e.at("repeat").get<int>();

                if (e.contains("source")) {
                    const auto &src = e.at("source");
                    if (src.contains("ref"))
                        entry.sourceRef = QString::fromStdString(src.at("ref").get<std::string>());
                    if (src.contains("source_name_ar"))
                        entry.sourceNameAr = QString::fromStdString(src.at("source_name_ar").get<std::string>());
                    if (src.contains("url"))
                        entry.sourceUrl = QString::fromStdString(src.at("url").get<std::string>());
                }

                m_all.append(entry);
            }
        }

        qInfo() << "[AdhkarDatabase] Loaded" << m_all.size() << "entries.";

    } catch (const json::exception &ex) {
        qWarning() << "[AdhkarDatabase] JSON parse error:" << ex.what();
        return false;
    }

    // Default: all categories active
    if (m_activeCategories.isEmpty()) {
        for (const auto &e : m_all) {
            if (!m_activeCategories.contains(e.categoryId))
                m_activeCategories.append(e.categoryId);
        }
    }

    rebuildActiveList();
    emit databaseReloaded();
    return true;
}

QStringList AdhkarDatabase::availableCategories() const
{
    QStringList cats;
    for (const auto &e : m_all) {
        if (!cats.contains(e.categoryId))
            cats.append(e.categoryId);
    }
    return cats;
}

void AdhkarDatabase::setActiveCategories(const QStringList &ids)
{
    m_activeCategories = ids;
    rebuildActiveList();
}

QStringList AdhkarDatabase::activeCategories() const
{
    return m_activeCategories;
}

void AdhkarDatabase::setShuffleEnabled(bool enabled)
{
    m_shuffle = enabled;
    if (m_shuffle)
        shuffleList();
}

bool AdhkarDatabase::shuffleEnabled() const { return m_shuffle; }

int AdhkarDatabase::count() const { return m_active.size(); }

const AdhkarEntry &AdhkarDatabase::next()
{
    if (m_active.isEmpty()) {
        static AdhkarEntry empty;
        return empty;
    }
    m_cursor = (m_cursor + 1) % m_active.size();
    // Re-shuffle when we wrap around
    if (m_cursor == 0 && m_shuffle)
        shuffleList();
    return m_all[m_active[m_cursor]];
}

const AdhkarEntry &AdhkarDatabase::current() const
{
    if (m_active.isEmpty()) {
        static AdhkarEntry empty;
        return empty;
    }
    return m_all[m_active[m_cursor]];
}

void AdhkarDatabase::reset()
{
    m_cursor = 0;
    if (m_shuffle)
        shuffleList();
}

// ── Private ──────────────────────────────────────────────────────────────────

void AdhkarDatabase::rebuildActiveList()
{
    m_active.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        if (m_activeCategories.contains(m_all[i].categoryId))
            m_active.append(i);
    }
    m_cursor = 0;
    if (m_shuffle)
        shuffleList();

    qInfo() << "[AdhkarDatabase] Active entries:" << m_active.size();
}

void AdhkarDatabase::shuffleList()
{
    // Fisher-Yates shuffle using Qt's cryptographically strong RNG
    for (int i = m_active.size() - 1; i > 0; --i) {
        int j = static_cast<int>(QRandomGenerator::global()->bounded(i + 1));
        std::swap(m_active[i], m_active[j]);
    }
}
