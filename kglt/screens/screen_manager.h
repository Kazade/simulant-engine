#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <unordered_map>
#include <functional>
#include "screen.h"

namespace kglt {

class WindowBase;

class ScreenManager : public Managed {
public:
    ScreenManager(WindowBase& window);

    template<typename T>
    void route(const unicode& route) {
        screen_factories_[route] = [=]() -> Screen::ptr {
            return T::create(window_);
        };
    }

    void redirect(const unicode& route);
    void background_load(const unicode& route);
    void unload(const unicode& route);
    bool is_loaded(const unicode& route) const;

private:
    WindowBase& window_;
    std::unordered_map<unicode, std::function<Screen::ptr ()>> screen_factories_;
    std::unordered_map<unicode, Screen::ptr> routes_;

    Screen::ptr current_screen_;

    Screen::ptr get_or_create_route(const unicode& route);

    struct BackgroundTask {
        unicode route;
        std::future<void> future;
    };
};

}

#endif // SCREEN_MANAGER_H
