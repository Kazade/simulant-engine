
#include "event_listener.h"
#include "core.h"
#include "macros.h"

namespace smlt {

void EventListener::handle_touch_begin(Window* window, TouchPointID touch_id, float normalized_x, float normalized_y, float pressure) {
    _S_UNUSED(pressure); // FIXME: Pass down pressure

    TouchEvent evt;
    evt.type = TOUCH_EVENT_TYPE_FINGER_DOWN;
    evt.touch_id = touch_id;
    evt.normalized_coord = Vec2(normalized_x, normalized_y);
    evt.coord = window->coordinate_from_normalized(normalized_x, normalized_y);

    on_touch_begin(evt);
}

void EventListener::handle_touch_end(Window* window, TouchPointID touch_id, float normalized_x, float normalized_y) {
    TouchEvent evt;
    evt.type = TOUCH_EVENT_TYPE_FINGER_UP;
    evt.touch_id = touch_id;
    evt.normalized_coord = Vec2(normalized_x, normalized_y);
    evt.coord = window->coordinate_from_normalized(normalized_x, normalized_y);

    on_touch_begin(evt);
}

void EventListener::handle_touch_move(Window* window, TouchPointID touch_id, float normalized_x, float normalized_y, float dx, float dy) {
    TouchEvent evt;
    evt.type = TOUCH_EVENT_TYPE_FINGER_MOVE;
    evt.touch_id = touch_id;
    evt.normalized_coord = Vec2(normalized_x, normalized_y);
    evt.coord = window->coordinate_from_normalized(normalized_x, normalized_y);
    evt.movement = Vec2(dx, dy);

    on_touch_move(evt);
}

void EventListener::handle_key_down(Window* window, KeyboardCode code, ModifierKeyState modifiers) {
    _S_UNUSED(window);

    KeyEvent evt;
    evt.type = KEY_EVENT_TYPE_KEY_DOWN;
    evt.keyboard_code = code;
    evt.modifiers = modifiers;

    on_key_down(evt);
}

void EventListener::handle_key_up(Window* window, KeyboardCode code, ModifierKeyState modifiers) {
    _S_UNUSED(window);

    KeyEvent evt;
    evt.type = KEY_EVENT_TYPE_KEY_UP;
    evt.keyboard_code = code;
    evt.modifiers = modifiers;

    on_key_up(evt);
}

void EventListenerManager::register_event_listener(EventListener* listener) {
    listeners_.push_back(listener);
}

void EventListenerManager::unregister_event_listener(EventListener* listener) {
    listeners_.remove(listener);
}

void EventListenerManager::each_event_listener(std::function<void (EventListener*)> callback) {
    auto listeners = listeners_; // Copy, in case listeners_ is altered in a callback

    for(auto lst: listeners) {
        // If a listener was removed don't try to call a callback on it
        if(std::find(listeners_.begin(), listeners_.end(), lst) == listeners_.end()) {
            continue;
        }

        callback(lst);
    }
}

}
