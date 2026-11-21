#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * A single Adhkar entry as loaded from adhkar.json.
 */
struct AdhkarEntry {
    QString id;
    QString categoryId;
    QString textAr;          ///< Arabic text (primary)
    QString transliteration; ///< Latin transliteration
    QString translationEn;   ///< English translation
    int     repeat = 1;      ///< Recommended repetition count
    QString sourceRef;       ///< e.g. "Bukhari 6306"
    QString sourceNameAr;    ///< e.g. "البخاري"
    QString sourceUrl;       ///< Link to Dorar/IslamWeb/IslamQA
};

/**
 * Loads the JSON database, filters by active categories,
 * and provides sequential or shuffled iteration.
 */
class AdhkarDatabase : public QObject
{
    Q_OBJECT

public:
    explicit AdhkarDatabase(QObject *parent = nullptr);

    /**
     * Load from a Qt resource path or filesystem path.
     * Returns true on success.
     */
    bool load(const QString &jsonPath);

    // ── Category filtering ────────────────────────────────────────────────
    QStringList availableCategories() const;
    void setActiveCategories(const QStringList &ids);
    QStringList activeCategories() const;

    // ── Iteration ─────────────────────────────────────────────────────────
    void setShuffleEnabled(bool enabled);
    bool shuffleEnabled() const;

    /** Total number of entries that pass the current filter. */
    int count() const;

    /** Advance and return the next entry (wraps around). */
    const AdhkarEntry &next();

    /** Peek at the current entry without advancing. */
    const AdhkarEntry &current() const;

    /** Reset the iterator to the beginning (or re-shuffle). */
    void reset();

signals:
    void databaseReloaded();

private:
    void rebuildActiveList();
    void shuffleList();

    // All entries loaded from JSON
    QVector<AdhkarEntry> m_all;

    // Filtered + possibly shuffled view (indices into m_all)
    QVector<int> m_active;

    QStringList m_activeCategories;
    bool        m_shuffle = false;
    int         m_cursor  = 0;
};
