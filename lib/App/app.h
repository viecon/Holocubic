#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "config.h"

class App
{
public:
    virtual ~App() = default;

    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void loop() = 0;
    virtual bool onTilt(int direction) { return false; }
    virtual const char *name() const = 0;

    void triggerOverlay()
    {
        _overlayVisible = true;
        _overlayTriggerTime = millis();
    }

    void updateOverlay()
    {
        if (_overlayVisible && (millis() - _overlayTriggerTime >= OVERLAY_SHOW_MS))
            _overlayVisible = false;
    }

    bool isOverlayVisible() const { return _overlayVisible; }

protected:
    bool _overlayVisible = false;
    unsigned long _overlayTriggerTime = 0;
};

#endif // APP_H
