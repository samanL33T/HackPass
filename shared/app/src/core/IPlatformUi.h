#pragma once

#include <QString>

class QWindow;

class IPlatformUi {
public:
    virtual ~IPlatformUi() = default;

    virtual void    applyAppStyleHints() = 0;
    virtual void    applyWindowChromeHints(QWindow* window) = 0;
    virtual void    registerGlobalHotkeys(QWindow* window) = 0;
    virtual QString defaultBrowserExecutable() const = 0;
};
