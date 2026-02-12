#ifndef GIF_ROUTES_H
#define GIF_ROUTES_H

#include <ESPAsyncWebServer.h>

namespace GifRoutes
{
    void registerRoutes(AsyncWebServer &server);
    void setOnGifChange(void (*callback)());
}

#endif // GIF_ROUTES_H
