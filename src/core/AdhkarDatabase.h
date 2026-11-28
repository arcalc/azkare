#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct AdhkarEntry {
    QString id;
    QString categoryId;
    QString textAr;
    QString translationEn;
    int repeat = 1;        ///< "count" in the JSON: how many times to recite
    QString sourceAr;      ///< e.g. "سورة البقرة: 255" or "رواه مسلم (2723) — حصن المسلم"
    QString sourceEn;      ///< e.g. "Surah Al-Baqarah 2:255" or "Sahih Muslim 2723 — Hisn al-Muslim"
    bool isValid() const { return !id.isEmpty(); }
    bool hasSource() const { return !sourceAr.isEmpty() && !sourceEn.isEmpty(); }
    /** Both sources are always shown together, whatever language the UI is in. */
    QString sourceLabelAr() const { return QString::fromUtf8("المصدر: ") + sourceAr; }
    QString sourceLabelEn() const { return QStringLiteral("Source: ") + sourceEn; }
};

struct AdhkarCategory {
    QString id;
    QString nameAr;
    QString nameEn;
    int count = 0;
};

/** Loads the JSON dataset, filters by category and iterates sequentially or shuffled. */
class AdhkarDatabase : public QObject
{
    Q_OBJECT
public:
    explicit AdhkarDatabase(QObject *parent = nullptr);

    bool load(const QString &path);
    int totalCount() const { return m_all.size(); }
    const QVector<AdhkarCategory> &categories() const { return m_cats; }
    QString categoryName(const QString &id, const QString &lang) const;

    /** Empty list (or a list matching nothing) means "all categories". */
    void setActiveCategories(const QStringList &ids);
    QStringList activeCategories() const { return m_activeCategories; }
    void setShuffleEnabled(bool on);

    int count() const { return m_active.size(); }
    const AdhkarEntry &current() const;
    const AdhkarEntry &next();
    void reset();

private:
    void rebuildActiveList();
    void shuffleList();

    QVector<AdhkarEntry> m_all;
    QVector<AdhkarCategory> m_cats;
    QVector<int> m_active;
    QStringList m_activeCategories;
    bool m_shuffle = false;
    int m_cursor = 0;
};
