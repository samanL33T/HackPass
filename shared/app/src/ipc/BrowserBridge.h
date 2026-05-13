#pragma once

#include <QHash>
#include <QObject>

class QWebSocket;
class QWebSocketServer;

class VaultManager;
class VaultModel;
class TokenHandshake;
class HardeningManager;

// Localhost WebSocket server the Chrome extension connects to. Dispatches
// JSON-RPC-shaped messages to vault and lock-state methods. Authenticates each
// client via TokenHandshake on the first message; subsequent messages reuse
// the same QWebSocket connection's authenticated state.
class BrowserBridge : public QObject {
    Q_OBJECT
public:
    BrowserBridge(VaultManager*    vaultManager,
                  VaultModel*      vaultModel,
                  TokenHandshake*  handshake,
                  HardeningManager* hardening,
                  QObject*         parent = nullptr);
    ~BrowserBridge() override;

    bool listen(quint16 port = 8765);
    void close();

    bool isListening() const;
    quint16 serverPort() const;

private slots:
    void onNewConnection();
    void onTextMessage(const QString& message);
    void onSocketDisconnected();

private:
    void send(QWebSocket* sock, const QJsonObject& obj);
    void handleHandshake(QWebSocket* sock, const QJsonObject& params, int id);
    void handleMethod(QWebSocket* sock, const QString& method, const QJsonObject& params, int id);

    QWebSocketServer* m_server   = nullptr;
    VaultManager*     m_vaultManager = nullptr;
    VaultModel*       m_vaultModel   = nullptr;
    TokenHandshake*   m_handshake    = nullptr;
    HardeningManager* m_hardening    = nullptr;

    QHash<QWebSocket*, bool> m_authenticated;  // socket -> auth flag
};
