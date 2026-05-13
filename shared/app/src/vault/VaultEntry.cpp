#include "VaultEntry.h"

#include <QJsonArray>
#include <QTimeZone>
#include <QUuid>

namespace {

void zeroString(QString& s) {
    if (s.isEmpty()) return;
    // Overwrite the underlying UTF-16 buffer in place.
    QChar* d = s.data();
    for (int i = 0; i < s.size(); ++i) {
        d[i] = QChar(0);
    }
    s.clear();
}

QDateTime parseUtc(const QString& iso) {
    auto dt = QDateTime::fromString(iso, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(iso, Qt::ISODate);
    }
    if (dt.isValid()) {
        dt.setTimeZone(QTimeZone(QTimeZone::UTC));
    }
    return dt;
}

}  // namespace

QJsonObject PasswordHistoryEntry::toJson() const {
    QJsonObject o;
    o["password"]   = password;
    o["changed_at"] = changedAt.toUTC().toString(Qt::ISODateWithMs);
    return o;
}

PasswordHistoryEntry PasswordHistoryEntry::fromJson(const QJsonObject& obj) {
    PasswordHistoryEntry p;
    p.password  = obj.value("password").toString();
    p.changedAt = parseUtc(obj.value("changed_at").toString());
    return p;
}

void VaultEntry::zero() {
    zeroString(username);
    zeroString(password);
    zeroString(totpSecret);
    zeroString(notes);
    for (auto& h : history) {
        zeroString(h.password);
    }
    history.clear();
}

QJsonObject VaultEntry::toJson() const {
    QJsonObject o;
    o["id"]          = id;
    o["type"]        = typeToString(type);
    o["title"]       = title;
    o["username"]    = username;
    o["password"]    = password;
    o["url"]         = url;
    o["totp_secret"] = totpSecret;
    o["notes"]       = notes;
    QJsonArray tagsArr;
    for (const auto& t : tags) tagsArr.append(t);
    o["tags"]       = tagsArr;
    o["favorite"]   = favorite;
    o["created_at"] = createdAt.toUTC().toString(Qt::ISODateWithMs);
    o["updated_at"] = updatedAt.toUTC().toString(Qt::ISODateWithMs);
    QJsonArray histArr;
    for (const auto& h : history) histArr.append(h.toJson());
    o["history"] = histArr;
    return o;
}

VaultEntry VaultEntry::fromJson(const QJsonObject& obj) {
    VaultEntry e;
    e.id         = obj.value("id").toString();
    e.type       = typeFromString(obj.value("type").toString());
    e.title      = obj.value("title").toString();
    e.username   = obj.value("username").toString();
    e.password   = obj.value("password").toString();
    e.url        = obj.value("url").toString();
    e.totpSecret = obj.value("totp_secret").toString();
    e.notes      = obj.value("notes").toString();
    for (const auto& v : obj.value("tags").toArray()) {
        e.tags.append(v.toString());
    }
    e.favorite  = obj.value("favorite").toBool();
    e.createdAt = parseUtc(obj.value("created_at").toString());
    e.updatedAt = parseUtc(obj.value("updated_at").toString());
    for (const auto& v : obj.value("history").toArray()) {
        e.history.append(PasswordHistoryEntry::fromJson(v.toObject()));
    }
    return e;
}

VaultEntry VaultEntry::makeLogin(const QString& title, const QString& username, const QString& password) {
    VaultEntry e;
    e.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.type      = Type::Login;
    e.title     = title;
    e.username  = username;
    e.password  = password;
    e.createdAt = QDateTime::currentDateTimeUtc();
    e.updatedAt = e.createdAt;
    return e;
}

QString VaultEntry::typeToString(Type t) {
    switch (t) {
        case Type::Login:      return QStringLiteral("login");
        case Type::SecureNote: return QStringLiteral("secure_note");
        case Type::Card:       return QStringLiteral("card");
        case Type::Identity:   return QStringLiteral("identity");
    }
    return QStringLiteral("login");
}

VaultEntry::Type VaultEntry::typeFromString(const QString& s) {
    if (s == QLatin1String("secure_note")) return Type::SecureNote;
    if (s == QLatin1String("card"))        return Type::Card;
    if (s == QLatin1String("identity"))    return Type::Identity;
    return Type::Login;
}
