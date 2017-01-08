/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WATCHER_H
#define WATCHER_H

#include <unordered_map>
#include "generic/managed.h"

#include "utils/unicode.h"

namespace smlt {

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
