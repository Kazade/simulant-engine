#include <SDL.h>
#include "platform.h"

namespace smlt {

std::string AndroidPlatform::name() const {
    return "android";
}

smlt::Resolution smlt::AndroidPlatform::native_resolution() const {
    SDL_DisplayMode mode;

    Resolution native;
    if(SDL_GetDesktopDisplayMode(0, &mode) == -1) {
        S_WARN("Unable to get the current desktop display mode!!");
        S_WARN("{0}", SDL_GetError());
        S_WARN("Falling back to 1080p");
        native.width = 1920;
        native.height = 1080;
        native.refresh_rate = 60;
    } else {
        native.width = mode.w;
        native.height = mode.h;
        native.refresh_rate = mode.refresh_rate;
    }
    return native;
}


}
