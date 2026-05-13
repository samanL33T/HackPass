#include "BrowserBridge.h"

#include "BrowserProtocol.h"
#include "OriginCheck.h"
#include "TokenHandshake.h"

#include "hardening/HardeningManager.h"
#include "vault/VaultEntry.h"
#include "vault/VaultManager.h"
#include "vault/VaultModel.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QWebSocketServer>

BrowserBridge::BrowserBridge(VaultManager*    vaultManager,
                             VaultModel*      vaultModel,
                             TokenHandshake*  handshake,
                             HardeningManager* hardening,
                             QObject*         parent)
    : QObject(parent),
      m_vaultManager(vaultManager),
      m_vaultModel(vaultModel),
      m_handshake(handshake),
      m_hardening(hardening) {
    m_server = new QWebSocketServer(QStringLiteral("HackPass IPC"),
                                    QWebSocketServer::NonSecureMode, this);
    connect(m_server, &QWebSocketServer::newConnection,
            this, &BrowserBridge::onNewConnection);
}

BrowserBridge::~BrowserBridge() {
    close();
}

bool BrowserBridge::listen(quint16 port) {
    if (m_server->isListening()) return true;
    return m_server->listen(QHostAddress::LocalHost, port);
}

void BrowserBridge::close() {
    if (m_server && m_server->isListening()) {
        m_server->close();
    }
    for (auto it = m_authenticated.begin(); it != m_authenticated.end(); ++it) {
        it.key()->deleteLater();
    }
    m_authenticated.clear();
}

bool BrowserBridge::isListening() const {
    return m_server && m_server->isListening();
}

quint16 BrowserBridge::serverPort() const {
    return m_server ? m_server->serverPort() : 0;
}

void BrowserBridge::onNewConnection() {
    if (m_hardening && m_hardening->checkAtCallsite(u"BrowserBridge::onAuthenticate")) {
        return;
    }
    while (QWebSocket* sock = m_server->nextPendingConnection()) {
        if (!OriginCheck::isAllowed(sock->origin())) {
            sock->close(QWebSocketProtocol::CloseCodePolicyViolated, "bad origin");
            sock->deleteLater();
            continue;
        }
        m_authenticated[sock] = false;
        connect(sock, &QWebSocket::textMessageReceived, this, &BrowserBridge::onTextMessage);
        connect(sock, &QWebSocket::disconnected,       this, &BrowserBridge::onSocketDisconnected);
    }
}

void BrowserBridge::onTextMessage(const QString& message) {
    auto* sock = qobject_cast<QWebSocket*>(sender());
    if (!sock) return;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        send(sock, BrowserProtocol::makeError(0, BrowserProtocol::ErrInvalidParams, "not json"));
        return;
    }
    auto req = BrowserProtocol::Request::fromJson(doc.object());
    if (!req.isValid()) {
        send(sock, BrowserProtocol::makeError(0, BrowserProtocol::ErrInvalidParams, "invalid request"));
        return;
    }
    if (req.method == QLatin1String("handshake")) {
        handleHandshake(sock, req.params, req.id);
        return;
    }
    if (!m_authenticated.value(sock, false)) {
        send(sock, BrowserProtocol::makeError(req.id, BrowserProtocol::ErrUnauthorized, "handshake required"));
        return;
    }
    handleMethod(sock, req.method, req.params, req.id);
}

void BrowserBridge::onSocketDisconnected() {
    auto* sock = qobject_cast<QWebSocket*>(sender());
    if (!sock) return;
    m_authenticated.remove(sock);
    sock->deleteLater();
}

void BrowserBridge::send(QWebSocket* sock, const QJsonObject& obj) {
    sock->sendTextMessage(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
}

void BrowserBridge::handleHandshake(QWebSocket* sock, const QJsonObject& params, int id) {
    const QString token = params.value("token").toString();
    const bool ok = m_handshake && m_handshake->validate(token);
    m_authenticated[sock] = ok;
    if (!ok) {
        send(sock, BrowserProtocol::makeError(id, BrowserProtocol::ErrUnauthorized, "token rejected"));
        return;
    }
    QJsonObject r;
    r["ok"] = true;
    send(sock, BrowserProtocol::makeResult(id, r));
}

void BrowserBridge::handleMethod(QWebSocket* sock, const QString& method, const QJsonObject& params, int id) {
    if (!m_vaultModel || !m_vaultManager) {
        send(sock, BrowserProtocol::makeError(id, -32000, "vault not ready"));
        return;
    }
    if (m_vaultManager->state() != VaultManager::State::Unlocked) {
        send(sock, BrowserProtocol::makeError(id, BrowserProtocol::ErrVaultLocked, "vault locked"));
        return;
    }

    if (method == QLatin1String("vault.lockState")) {
        QJsonObject r;
        r["state"] = static_cast<int>(m_vaultManager->state());
        send(sock, BrowserProtocol::makeResult(id, r));
        return;
    }
    if (method == QLatin1String("vault.findEntries")) {
        const QString url = params.value("url").toString();
        QJsonArray arr;
        for (const auto& e : m_vaultModel->entries()) {
            if (!url.isEmpty() && !e.url.contains(url, Qt::CaseInsensitive)) continue;
            QJsonObject o;
            o["id"]       = e.id;
            o["title"]    = e.title;
            o["username"] = e.username;
            o["url"]      = e.url;
            arr.append(o);
        }
        QJsonObject r;
        r["entries"] = arr;
        send(sock, BrowserProtocol::makeResult(id, r));
        return;
    }
    if (method == QLatin1String("vault.getEntry")) {
        const QString eid = params.value("id").toString();
        const auto entry = m_vaultModel->findById(eid);
        if (!entry.has_value()) {
            send(sock, BrowserProtocol::makeError(id, -32001, "not found"));
            return;
        }
        QJsonObject r;
        r["id"]          = entry->id;
        r["title"]       = entry->title;
        r["username"]    = entry->username;
        r["password"]    = entry->password;
        r["url"]         = entry->url;
        r["totp_secret"] = entry->totpSecret;
        send(sock, BrowserProtocol::makeResult(id, r));
        return;
    }
    if (method == QLatin1String("vault.saveEntry")) {
        const QString title    = params.value("title").toString();
        const QString username = params.value("username").toString();
        const QString password = params.value("password").toString();
        const QString url      = params.value("url").toString();

        // Dedup: if an entry already exists for this (host, username), update its
        // password in place (the old password is pushed onto history by updateLogin).
        // Otherwise insert a new entry.
        const QString existingId = m_vaultModel->findIdByHostAndUsername(url, username);
        QString resultId;
        bool    updated = false;
        if (!existingId.isEmpty()) {
            const auto existing = m_vaultModel->findById(existingId);
            const QString keepTitle = (existing && !existing->title.isEmpty()) ? existing->title : title;
            const QString keepUrl   = (existing && !existing->url.isEmpty())   ? existing->url   : url;
            const QString keepNotes = existing ? existing->notes : QString();
            if (m_vaultModel->updateLogin(existingId, keepTitle, username, password, keepUrl, keepNotes)) {
                resultId = existingId;
                updated  = true;
            }
        }
        if (resultId.isEmpty()) {
            resultId = m_vaultModel->addLogin(title, username, password, url, QString());
        }

        // Persist to disk. Without this the browser-saved entry is lost on restart.
        if (m_vaultManager && !m_vaultManager->save()) {
            send(sock, BrowserProtocol::makeError(id, -32000, "could not persist vault"));
            return;
        }

        QJsonObject r;
        r["id"]      = resultId;
        r["updated"] = updated;
        send(sock, BrowserProtocol::makeResult(id, r));
        return;
    }
    send(sock, BrowserProtocol::makeError(id, BrowserProtocol::ErrUnknownMethod, "unknown method"));
}
