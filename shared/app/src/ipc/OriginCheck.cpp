#include "OriginCheck.h"

bool OriginCheck::isAllowed(const QString& origin) {
    return origin.contains(QStringLiteral("chrome-extension://"));
}
