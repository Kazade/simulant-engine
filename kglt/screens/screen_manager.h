#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <unordered_map>
#include <functional>
#include <future>
#include "screen.h"

namespace kglt {

class WindowBase;

class ScreenManager : public Managed<ScreenManager> {
public:
    ScreenManager(WindowBase& window);

    template<typename T>
    void add_route(const unicode& route) {
        screen_factories_[route] = [=]() -> ScreenBase::ptr {
            return T::create(window_);
        };
    }

    bool has_route(const unicode& route) const;
    ScreenBase::ptr resolve(const unicode& route);
    void redirect(const unicode& route);
    void background_load(const unicode& route);
    void unload(const unicode& route);
    bool is_loaded(const unicode& route) const;
    void reset();

private:
    WindowBase& window_;
    std::unordered_map<unicode, std::function<ScreenBase::ptr ()>> screen_factories_;
    std::unordered_map<unicode, ScreenBase::ptr> routes_;

    ScreenBase::ptr current_screen_;

    ScreenBase::ptr get_or_create_route(const unicode& route);

    struct BackgroundTask {
        unicode route;
        std::future<void> future;
    };
};

}

#endif // SCREEN_MANAGER_H
