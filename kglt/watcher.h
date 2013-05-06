#ifndef WATCHER_H
#define WATCHER_H

namespace kglt {

/**
  USAGE:

  Watcher::ptr watcher = Watcher::create();

  typedef std::function<void (std::string, EventType)> WatchCallback;

  watcher->watch("path/to/file", &Thing::stuff);
  watcher->unwatch("path/to/file");
*/



}

#endif // WATCHER_H
