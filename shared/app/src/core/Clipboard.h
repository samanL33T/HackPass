#pragma once

#include <QObject>
#include <QString>

// Minimal QGuiApplication::clipboard wrapper exposed to QML as a context property.
// QML cannot reach the system clipboard via Qt::imports alone in Qt 6.x without
// pulling in QtCore.SystemClipboard - this wrapper keeps the import surface stable.
class Clipboard : public QObject {
    Q_OBJECT
public:
    explicit Clipboard(QObject* parent = nullptr);

    Q_INVOKABLE void    setText(const QString& text);
    Q_INVOKABLE QString text() const;
};
