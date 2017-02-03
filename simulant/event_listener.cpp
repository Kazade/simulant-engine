
#include "event_listener.h"
#include "window_base.h"

namespace smlt {

void EventListener::handle_touch_begin(WindowBase* window, TouchPointID touch_id, float normalized_x, float normalized_y, float pressure) {
    TouchEvent evt;
    evt.type = TOUCH_EVENT_TYPE_FINGER_DOWN;
    evt.touch_id = touch_id;
    evt.normalized_coord = Vec2(normalized_x, normalized_y);
    evt.coord = window->coordinate_from_normalized(normalized_x, normalized_y);

    on_touch_begin(evt);
}

void EventListener::handle_touch_end(WindowBase* window, TouchPointID touch_id, float normalized_x, float normalized_y) {
    TouchEvent evt;
    evt.type = TOUCH_EVENT_TYPE_FINGER_UP;
    evt.touch_id = touch_id;
    evt.normalized_coord = Vec2(normalized_x, normalized_y);
    evt.coord = window->coordinate_from_normalized(normalized_x, normalized_y);

    on_touch_begin(evt);
}

void EventListener::handle_touch_move(WindowBase* window, TouchPointID touch_id, float normalized_x, float normalized_y, float dx, float dy) {
    TouchEvent evt;
    evt.type = TOUCH_EVENT_TYPE_FINGER_MOVE;
    evt.touch_id = touch_id;
    evt.normalized_coord = Vec2(normalized_x, normalized_y);
    evt.coord = window->coordinate_from_normalized(normalized_x, normalized_y);
    evt.movement = Vec2(dx, dy);

    on_touch_move(evt);
}

void EventListenerManager::register_event_listener(EventListener* listener) {
    listeners_.push_back(listener);
}

void EventListenerManager::unregister_event_listener(EventListener* listener) {
    listeners_.remove(listener);
}

void EventListenerManager::each_event_listener(std::function<void (EventListener*)> callback) {
    for(auto lst: listeners_) {
        callback(lst);
    }
}

}
