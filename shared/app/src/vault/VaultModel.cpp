#include "VaultModel.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {

QString hostOf(const QString& url) {
    if (url.isEmpty()) return {};
    QUrl u = QUrl::fromUserInput(url);
    const QString h = u.host(QUrl::FullyDecoded);
    return h.isEmpty() ? url.toLower() : h.toLower();
}

}  // namespace

VaultModel::VaultModel(QObject* parent) : QAbstractListModel(parent) {}

int VaultModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant VaultModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const VaultEntry& e = m_entries.at(index.row());
    switch (role) {
        case IdRole:        return e.id;
        case TypeRole:      return VaultEntry::typeToString(e.type);
        case TitleRole:     return e.title;
        case UsernameRole:  return e.username;
        case UrlRole:       return e.url;
        case FavoriteRole:  return e.favorite;
        case TagsRole:      return e.tags;
        case UpdatedAtRole: return e.updatedAt;
        case Qt::DisplayRole: return e.title;
        default:            return {};
    }
}

QHash<int, QByteArray> VaultModel::roleNames() const {
    return {
        { IdRole,        "id" },
        { TypeRole,      "type" },
        { TitleRole,     "title" },
        { UsernameRole,  "username" },
        { UrlRole,       "url" },
        { FavoriteRole,  "favorite" },
        { TagsRole,      "tags" },
        { UpdatedAtRole, "updatedAt" },
    };
}

void VaultModel::replaceAll(QList<VaultEntry> entries) {
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
}

void VaultModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int VaultModel::addEntry(const VaultEntry& e) {
    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append(e);
    endInsertRows();
    return row;
}

bool VaultModel::removeEntry(const QString& id) {
    const int idx = indexOfId(id);
    if (idx < 0) return false;
    beginRemoveRows(QModelIndex(), idx, idx);
    m_entries[idx].zero();
    m_entries.removeAt(idx);
    endRemoveRows();
    return true;
}

bool VaultModel::updateEntry(const VaultEntry& e) {
    const int idx = indexOfId(e.id);
    if (idx < 0) return false;
    m_entries[idx] = e;
    const QModelIndex mi = index(idx);
    emit dataChanged(mi, mi);
    return true;
}

std::optional<VaultEntry> VaultModel::findById(const QString& id) const {
    const int idx = indexOfId(id);
    if (idx < 0) return std::nullopt;
    return m_entries.at(idx);
}

QVariantMap VaultModel::entryById(const QString& id) const {
    const int idx = indexOfId(id);
    if (idx < 0) return {};
    const VaultEntry& e = m_entries.at(idx);
    QVariantMap m;
    m["id"]          = e.id;
    m["type"]        = VaultEntry::typeToString(e.type);
    m["title"]       = e.title;
    m["username"]    = e.username;
    m["password"]    = e.password;
    m["url"]         = e.url;
    m["totp_secret"] = e.totpSecret;
    m["notes"]       = e.notes;
    m["tags"]        = e.tags;
    m["favorite"]    = e.favorite;
    return m;
}

QString VaultModel::addLogin(const QString& title,
                             const QString& username,
                             const QString& password,
                             const QString& url,
                             const QString& notes,
                             const QString& totpSecret) {
    if (title.trimmed().isEmpty()) return {};
    VaultEntry e  = VaultEntry::makeLogin(title.trimmed(), username, password);
    e.url         = url;
    e.notes       = notes;
    e.totpSecret  = totpSecret.trimmed();
    addEntry(e);
    return e.id;
}

bool VaultModel::removeEntryById(const QString& id) {
    return removeEntry(id);
}

bool VaultModel::updateLogin(const QString& id,
                             const QString& title,
                             const QString& username,
                             const QString& password,
                             const QString& url,
                             const QString& notes,
                             const QString& totpSecret) {
    auto existing = findById(id);
    if (!existing.has_value()) return false;
    VaultEntry e = *existing;
    if (e.password != password) {
        PasswordHistoryEntry h;
        h.password  = e.password;
        h.changedAt = QDateTime::currentDateTimeUtc();
        e.history.prepend(h);
    }
    e.title      = title.trimmed();
    e.username   = username;
    e.password   = password;
    e.url        = url;
    e.notes      = notes;
    e.totpSecret = totpSecret.trimmed();
    e.updatedAt  = QDateTime::currentDateTimeUtc();
    return updateEntry(e);
}

bool VaultModel::exportToJson(const QString& path) const {
    QJsonArray arr;
    for (const auto& e : m_entries) {
        arr.append(e.toJson());
    }
    QJsonObject root;
    root["exported_at"]    = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    root["schema_version"] = QStringLiteral("1");
    root["entry_count"]    = m_entries.size();
    root["entries"]        = arr;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    const qint64 written  = f.write(json);
    f.close();
    return written == json.size();
}

QString VaultModel::findIdByHostAndUsername(const QString& url, const QString& username) const {
    // Username is required for a meaningful match. Sites with password-only login
    // (empty username) intentionally do not dedup; the caller will insert a new entry.
    if (username.isEmpty()) return {};
    const QString needle = hostOf(url);
    if (needle.isEmpty()) return {};
    for (const auto& e : m_entries) {
        if (e.type != VaultEntry::Type::Login) continue;
        if (e.username != username) continue;
        if (hostOf(e.url) == needle) {
            return e.id;
        }
    }
    return {};
}

QList<VaultEntry> VaultModel::take() {
    QList<VaultEntry> out;
    out.swap(m_entries);
    beginResetModel();
    endResetModel();
    return out;
}

void VaultModel::zeroAll() {
    beginResetModel();
    for (auto& e : m_entries) {
        e.zero();
    }
    m_entries.clear();
    endResetModel();
}

int VaultModel::indexOfId(const QString& id) const {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).id == id) return i;
    }
    return -1;
}
