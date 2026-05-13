#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QJsonObject>

struct PasswordHistoryEntry {
    QString    password;
    QDateTime  changedAt;

    QJsonObject toJson() const;
    static PasswordHistoryEntry fromJson(const QJsonObject& obj);
};

struct VaultEntry {
    enum class Type {
        Login      = 0,
        SecureNote = 1,
        Card       = 2,
        Identity   = 3,
    };

    QString    id;          // uuid (lowercase, no braces)
    Type       type = Type::Login;
    QString    title;
    QString    username;
    QString    password;
    QString    url;
    QString    totpSecret;  // base32, empty if none
    QString    notes;
    QStringList tags;
    bool       favorite = false;
    QDateTime  createdAt;
    QDateTime  updatedAt;
    QList<PasswordHistoryEntry> history;

    // Zero-fill every secret-bearing field so the entry can no longer be read
    // from memory after a vault lock. Best-effort; managed string copies may linger.
    void zero();

    QJsonObject toJson() const;
    static VaultEntry fromJson(const QJsonObject& obj);

    static VaultEntry makeLogin(const QString& title, const QString& username, const QString& password);

    static QString typeToString(Type t);
    static Type    typeFromString(const QString& s);
};
