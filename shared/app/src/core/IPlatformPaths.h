#pragma once

#include <QString>

class IPlatformPaths {
public:
    virtual ~IPlatformPaths() = default;

    virtual QString settingsScopeKey() const = 0;
    virtual QString defaultVaultDir() const = 0;
    virtual QString configDir() const = 0;
    virtual QString cacheDir() const = 0;
    virtual QString ipcEndpointDescriptor() const = 0;
    virtual QString tofuStorePath() const = 0;
};
