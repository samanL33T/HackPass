#pragma once

#include <QAbstractListModel>
#include <QVariantMap>

#include "VaultEntry.h"

class VaultModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        TitleRole,
        UsernameRole,
        UrlRole,
        FavoriteRole,
        TagsRole,
        UpdatedAtRole,
    };

    explicit VaultModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void replaceAll(QList<VaultEntry> entries);
    void clear();

    int                addEntry(const VaultEntry& e);
    bool               removeEntry(const QString& id);
    bool               updateEntry(const VaultEntry& e);

    const QList<VaultEntry>& entries() const { return m_entries; }
    std::optional<VaultEntry> findById(const QString& id) const;

    // QML-friendly lookup. Returns a QVariantMap with all the entry's fields
    // (or an empty map if id is unknown). QML calls this from EntryDetailPage.
    Q_INVOKABLE QVariantMap entryById(const QString& id) const;

    // QML-friendly create. Returns the new entry's id or empty on failure.
    // Callers (EntryEditPage) then call vaultManager.save() to persist.
    Q_INVOKABLE QString addLogin(const QString& title,
                                 const QString& username,
                                 const QString& password,
                                 const QString& url,
                                 const QString& notes,
                                 const QString& totpSecret = QString());

    Q_INVOKABLE bool removeEntryById(const QString& id);
    Q_INVOKABLE bool updateLogin(const QString& id,
                                 const QString& title,
                                 const QString& username,
                                 const QString& password,
                                 const QString& url,
                                 const QString& notes,
                                 const QString& totpSecret = QString());

    // Returns the id of the first entry whose URL has the same host as `url`
    // and whose username matches `username` (case-sensitive). Empty if no match.
    // Used by BrowserBridge::vault.saveEntry to dedup browser-driven saves.
    Q_INVOKABLE QString findIdByHostAndUsername(const QString& url, const QString& username) const;

    // Premium-gated: dump every entry (including passwords and history) as a
    // JSON file at `path`. Returns true on success. The export is plaintext
    // by design - it is the canonical premium feature plus the textbook lesson
    // target for runtime response-tampering and bridge bypass research.
    Q_INVOKABLE bool exportToJson(const QString& path) const;

    // For sync / lock: produce a list to pass to VaultFile::save, zero internal state.
    QList<VaultEntry> take();
    void              zeroAll();

private:
    int indexOfId(const QString& id) const;

    QList<VaultEntry> m_entries;
};
