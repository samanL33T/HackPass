#pragma once

#include "core/IPlatformPaths.h"

class PlatformPathsWindows : public IPlatformPaths {
public:
    QString settingsScopeKey() const override;
    QString defaultVaultDir() const override;
    QString configDir() const override;
    QString cacheDir() const override;
    QString ipcEndpointDescriptor() const override;
    QString tofuStorePath() const override;
};
