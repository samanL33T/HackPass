#pragma once

#include "core/IPlatformUi.h"

class PlatformUiMacOS : public IPlatformUi {
public:
    void    applyAppStyleHints() override;
    void    applyWindowChromeHints(QWindow* window) override;
    void    registerGlobalHotkeys(QWindow* window) override;
    QString defaultBrowserExecutable() const override;
};
