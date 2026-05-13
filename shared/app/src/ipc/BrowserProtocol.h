#pragma once

#include <QJsonObject>
#include <QString>

// JSON-RPC-shaped messages over the BrowserBridge WebSocket.
//
// Request : { "jsonrpc": "2.0", "id": <int>, "method": <string>, "params": {...} }
// Response: { "jsonrpc": "2.0", "id": <int>, "result": {...} }
//        OR { "jsonrpc": "2.0", "id": <int>, "error":  { "code": int, "message": string } }
namespace BrowserProtocol {

struct Request {
    int         id = 0;
    QString     method;
    QJsonObject params;

    static Request fromJson(const QJsonObject& obj);
    bool           isValid() const { return id > 0 && !method.isEmpty(); }
};

QJsonObject makeResult(int id, const QJsonObject& result);
QJsonObject makeError(int id, int code, const QString& message);

// Pre-defined error codes.
constexpr int ErrUnauthorized   = -32001;
constexpr int ErrVaultLocked    = -32002;
constexpr int ErrUnknownMethod  = -32601;
constexpr int ErrInvalidParams  = -32602;

}  // namespace BrowserProtocol
