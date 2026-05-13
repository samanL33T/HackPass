#include "Clipboard.h"

#include <QClipboard>
#include <QGuiApplication>

Clipboard::Clipboard(QObject* parent) : QObject(parent) {}

void Clipboard::setText(const QString& text) {
    if (QClipboard* cb = QGuiApplication::clipboard()) {
        cb->setText(text);
    }
}

QString Clipboard::text() const {
    if (QClipboard* cb = QGuiApplication::clipboard()) {
        return cb->text();
    }
    return {};
}
