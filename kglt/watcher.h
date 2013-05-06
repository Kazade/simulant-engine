#ifndef WATCHER_H
#define WATCHER_H

#include <unordered_map>
#include "generic/managed.h"
#include "kazbase/unicode.h"

namespace kglt {

/**
  USAGE:

  Watcher::ptr watcher = Watcher::create();

  typedef std::function<void (std::string, EventType)> WatchCallback;

  watcher->watch("path/to/file", &Thing::stuff);
  watcher->unwatch("path/to/file");
*/

class WindowBase;

enum WatchEvent {
    WATCH_EVENT_MODIFY,
    WATCH_EVENT_DELETE,
    WATCH_EVENT_MOVE
};

typedef std::function<void (unicode, WatchEvent)> WatchCallback;

class Watcher:
    public Managed<Watcher> {

public:
    Watcher();
    Watcher(WindowBase &window);

    ~Watcher();

    bool init();

    void watch(const unicode& path, WatchCallback cb);
    void unwatch(const unicode& path);

    bool update();

    void start();
    void stop();
private:
    WindowBase* window_;

    int inotify_fd_;
    bool watching_;
    int inotify_flag_;

    std::unordered_map<int, unicode> descriptor_paths_;
    std::unordered_map<unicode, int> watch_descriptors_;
    std::unordered_map<int, WatchCallback> watch_callbacks_;
};

}

#endif // WATCHER_H
