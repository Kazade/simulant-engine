#pragma once

#include <list>
#include "types.h"

namespace smlt {

/* EventListeners can directly handle events received by the window
 * (e.g. input)
 *
 *  An EventListener must be properly registered/unregistered with the window
 * with window->register_event_listener() and window->unregister_event_listener()
 */

typedef uint32_t TouchPointID;

enum TouchEventType {
    TOUCH_EVENT_TYPE_FINGER_DOWN,
    TOUCH_EVENT_TYPE_FINGER_UP,
    TOUCH_EVENT_TYPE_FINGER_MOVE
};

struct TouchEvent {
    TouchEventType type;
    TouchPointID touch_id;
    Vec2 coord;
    Vec2 normalized_coord;
    float pressure;
};

class EventListener {
public:
    EventListener() {}
    virtual ~EventListener() {}

    void handle_touch_begin(WindowBase* window, TouchPointID touch_id, float normalized_x, float normalized_y, float pressure);
    void handle_touch_end(WindowBase* window, TouchPointID touch_id, float normalized_x, float normalized_y);

private:
    virtual void on_touch_begin(const TouchEvent& evt) {}
    virtual void on_touch_end(const TouchEvent& evt) {}

    virtual void on_window_focus() {}
    virtual void on_window_blur() {}
    virtual void on_window_minimize() {}
    virtual void on_window_restore() {}
};

class EventListenerManager {
public:
    EventListenerManager() {}
    virtual ~EventListenerManager() {}

    void register_event_listener(EventListener* listener);
    void unregister_event_listener(EventListener* listener);

    void each_event_listener(std::function<void (EventListener*)> callback);

private:
    std::list<EventListener*> listeners_;
};

}
