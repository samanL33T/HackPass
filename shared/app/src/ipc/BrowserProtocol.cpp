#include "BrowserProtocol.h"

namespace BrowserProtocol {

Request Request::fromJson(const QJsonObject& obj) {
    Request r;
    r.id     = obj.value("id").toInt();
    r.method = obj.value("method").toString();
    r.params = obj.value("params").toObject();
    return r;
}

QJsonObject makeResult(int id, const QJsonObject& result) {
    QJsonObject o;
    o["jsonrpc"] = "2.0";
    o["id"]      = id;
    o["result"]  = result;
    return o;
}

QJsonObject makeError(int id, int code, const QString& message) {
    QJsonObject err;
    err["code"]    = code;
    err["message"] = message;
    QJsonObject o;
    o["jsonrpc"] = "2.0";
    o["id"]      = id;
    o["error"]   = err;
    return o;
}

}  // namespace BrowserProtocol
