#include "Logging.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#include <cstdio>
#include <cstdlib>

namespace {

QString g_logPath;
QMutex g_logMutex;

const char* labelFor(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:    return "debug";
        case QtInfoMsg:     return "info";
        case QtWarningMsg:  return "warn";
        case QtCriticalMsg: return "error";
        case QtFatalMsg:    return "fatal";
    }
    return "info";
}

void fileMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    Q_UNUSED(ctx);
    QMutexLocker locker(&g_logMutex);

    const QString ts   = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    const QString line = QStringLiteral("[%1] [%2] %3\n")
                             .arg(ts, QString::fromLatin1(labelFor(type)), msg);
    const QByteArray utf8 = line.toUtf8();

    if (!g_logPath.isEmpty()) {
        QFile log(g_logPath);
        if (log.open(QIODevice::Append | QIODevice::Text)) {
            log.write(utf8);
        }
    }

    std::fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stderr);
    std::fflush(stderr);

    if (type == QtFatalMsg) {
        std::abort();
    }
}

}  // namespace

void installFileMessageHandler(const QString& logFilePath) {
    g_logPath = logFilePath;
    const QFileInfo info(g_logPath);
    QDir().mkpath(info.absolutePath());
    qInstallMessageHandler(&fileMessageHandler);
}
