#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct AdhkarEntry {
    QString id;
    QString categoryId;
    QString textAr;
    QString transliteration;
    QString translationEn;
    int repeat = 1;
    QString sourceRef;     ///< e.g. "Muslim 2723"
    QString sourceNameAr;  ///< e.g. "مسلم"
    QString sourceUrl;     ///< optional
    bool isValid() const { return !id.isEmpty(); }
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
