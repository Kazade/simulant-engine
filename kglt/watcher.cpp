#include "watcher.h"
#include "window_base.h"

#include <sys/inotify.h>
#include <fcntl.h>

#include "kazbase/os.h"

namespace kglt {

/**
 * TODO: This could be done in a separate thread. We'd block
 * on read, rather than using poll() and then queue the callback on idle()
 */

Watcher::Watcher():
    window_(nullptr),
    inotify_fd_(0),
    watching_(false),
    inotify_flag_(0) {

    inotify_fd_ = inotify_init();
    inotify_flag_ = fcntl(inotify_fd_, F_GETFL, 0);
}

Watcher::Watcher(WindowBase& window):
    window_(&window),
    inotify_fd_(0),
    watching_(false),
    inotify_flag_(0) {

    inotify_fd_ = inotify_init();
    inotify_flag_ = fcntl(inotify_fd_, F_GETFL, 0);
}

Watcher::~Watcher() {

    std::unordered_map<int, unicode> tmp = descriptor_paths_;
    //Clean up
    for(const std::pair<int, unicode>& p: tmp) {
        try {
            unwatch(p.second);
        } catch(...) {
            L_ERROR("An error occurred when shutting down the file watcher");
        }
    }
}

void Watcher::start() {
    if(watching_) return;

    watching_ = true;

    if(window_) {
        window_->idle().add(std::bind(&Watcher::update, this));
    }
}

void Watcher::stop() {
    watching_ = false;
}

bool Watcher::init() {
    start();

    return true;
}

bool Watcher::update() {
    const int BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];

    fcntl(inotify_fd_, F_SETFL, inotify_flag_ | O_NONBLOCK); //Turn on non-blocking mode

    int got = read(inotify_fd_, buffer, BUFFER_SIZE);

    char* cur = buffer;
    char* end = buffer + got;

    while(cur < end) {
        struct inotify_event *ev = (struct inotify_event*)cur;
        cur += sizeof(struct inotify_event) + ev->len;

        while (ev->len > 0 && !ev->name[ev->len - 1]) {
            --ev->len;
        }

        int wd = ev->wd;

        WatchEvent evt;
        if(ev->mask == IN_MODIFY || ev->mask == IN_ATTRIB) {
            evt = WATCH_EVENT_MODIFY;
        } else if(ev->mask == IN_MOVE_SELF) {
            evt = WATCH_EVENT_MOVE;
        } else if(ev->mask == IN_DELETE_SELF) {
            evt = WATCH_EVENT_DELETE;
        } else if(ev->mask == IN_IGNORED) {
            continue;
        } else {
            throw LogicError("Received invalid mask from inotify");
        }

        watch_callbacks_.at(wd)(
            descriptor_paths_.at(wd),
            evt
        );
    }

    fcntl(inotify_fd_, F_SETFL, inotify_flag_); //Turn off non-blocking mode

    return watching_; //When this changes to false, it'll be removed from idle()
}

void Watcher::watch(const unicode &path, WatchCallback cb) {
    unicode p = os::path::abs_path(path);

    if(!os::path::exists(p)) {
        return;
    }

    int mask = IN_MODIFY | IN_ATTRIB | IN_MOVE_SELF | IN_DELETE_SELF;

    int wd = inotify_add_watch(inotify_fd_, p.encode().c_str(), mask);

    watch_callbacks_[wd] = cb;
    watch_descriptors_[path] = wd;
    descriptor_paths_[wd] = p;
}

void Watcher::unwatch(const unicode &path) {
    unicode p = os::path::abs_path(path);

    if(watch_descriptors_.find(p) != watch_descriptors_.end()) {
        int wd = watch_descriptors_.at(p);
        inotify_rm_watch(inotify_fd_, wd);
        watch_descriptors_.erase(p);
        watch_callbacks_.erase(wd);
        descriptor_paths_.erase(wd);
    }
}

}
