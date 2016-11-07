#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <unordered_map>
#include <functional>
#include <future>

#include "../generic/managed.h"
#include "../deps/kazsignal/kazsignal.h"

namespace smlt {

class WindowBase;
class ScreenBase;

typedef std::shared_ptr<ScreenBase> ScreenBasePtr;
typedef std::function<ScreenBasePtr (WindowBase&)> ScreenFactory;

template<typename T>
ScreenFactory screen_factory() {
    ScreenFactory ret = &T::template create<WindowBase&>;
    return ret;
}


class ScreenManagerInterface {
public:
    virtual ~ScreenManagerInterface() {}

    virtual void register_screen(const std::string& route, ScreenFactory factory) = 0;
    virtual bool has_screen(const std::string& route) const = 0;
    virtual ScreenBasePtr resolve_screen(const std::string& route) = 0;
    virtual void activate_screen(const std::string& route) = 0;
    virtual void load_screen_in_background(const std::string& route, bool redirect_after=true) = 0;
    virtual void unload_screen(const std::string& route) = 0;
    virtual bool is_screen_loaded(const std::string& route) const = 0;
    virtual ScreenBasePtr active_screen() const = 0;
    virtual const std::unordered_map<std::string, ScreenBasePtr> routes() const = 0;
};


class ScreenManager :
    public Managed<ScreenManager>,
    public ScreenManagerInterface {

public:
    ScreenManager(WindowBase& window);
    ~ScreenManager();

    void register_screen(const std::string& route, ScreenFactory factory);
    bool has_screen(const std::string& route) const;
    ScreenBasePtr resolve_screen(const std::string& route);
    void activate_screen(const std::string& route);
    void load_screen_in_background(const std::string& route, bool redirect_after=true);
    void unload_screen(const std::string& route);
    bool is_screen_loaded(const std::string& route) const;
    void reset();
    ScreenBasePtr active_screen() const;

    const std::unordered_map<std::string, ScreenBasePtr> routes() const {
        return routes_;
    }
private:
    WindowBase& window_;
    std::unordered_map<std::string, std::function<ScreenBasePtr ()>> screen_factories_;
    std::unordered_map<std::string, ScreenBasePtr> routes_;

    ScreenBasePtr current_screen_;

    ScreenBasePtr get_or_create_route(const std::string& route);

    struct BackgroundTask {
        std::string route;
        std::future<void> future;
    };

    sig::connection step_conn_;

    void step(double dt);
};

}

#endif // SCREEN_MANAGER_H
