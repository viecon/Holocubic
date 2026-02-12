#ifndef APP_H
#define APP_H

#include <Arduino.h>

class App
{
public:
    virtual ~App() = default;

    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void loop() = 0;

    virtual bool onTilt(int direction) { return false; }

    virtual const char *name() const = 0;
};

#endif // APP_H
