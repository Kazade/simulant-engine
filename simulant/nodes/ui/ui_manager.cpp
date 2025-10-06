
/*
 * Widget Input Explanation
 *
 * Widget input is hard. Widgets can be in any stage, be transformed in any way
 * and can be rendered with any camera to any viewport. That makes processing
 * input events tricky as the coordinates for touches/clicks are in window
 * coordinates and we won't know how to transform them until we process the
 * render queue.
 *
 * So, we tackle this by making the UIManager an event listener. When we receive
 * events we care about, we store them in a queue.
 *
 * The queue is processed each time the owning stage is rendered, but the queue
 * is not cleared! This is because you can render the same stage multiple times.
 * The queue is cleared at the end of each frame.
 */

#include "ui_manager.h"
#include "button.h"
#include "frame.h"
#include "image.h"
#include "keyboard.h"
#include "label.h"
#include "progress_bar.h"

#include "../../application.h"
#include "../../stage.h"
#include "../../vfs.h"
#include "../../viewport.h"
#include "../../window.h"
#include "../camera.h"
#include "../stage_node_manager.h"
#include "simulant/nodes/ui/ui_config.h"
#include "simulant/utils/params.h"

namespace smlt {
namespace ui {

using namespace std::placeholders;

UIManager::UIManager(Scene* owner, UIConfig config) :
    StageNode(owner, STAGE_NODE_TYPE_UI_MANAGER), config_(config) {

    auto window = get_app()->window.get();
    window->register_event_listener(this);

    /* We clear queued events at the end of each frame */
    frame_finished_connection_ =
        get_app()->signal_frame_finished().connect([this]() {
        this->clear_event_queue();
    });
}

UIManager::~UIManager() {
    pre_render_connection_.disconnect();
    frame_finished_connection_.disconnect();

    auto app = get_app();
    if(app) {
        auto window = app->window.get();

        if(window) {
            window->unregister_event_listener(this);
        }
    }
}

bool UIManager::on_create(Params params) {
    config_ = params.get<UIConfig>("config").value_or(UIConfig());
    return StageNode::on_create(params);
}

// Keyboard* UIManager::new_widget_as_keyboard(const KeyboardMode& mode, const
// unicode &initial_text) {
//     auto keyboard = manager_->make_as<Keyboard>(this, &config_, stage_, mode,
//     initial_text); stage_->append_child(keyboard); return keyboard;
// }

// Frame* UIManager::new_widget_as_frame(const unicode& title, const Px& width,
// const Px& height) {
//     auto frame = manager_->make_as<Frame>(this, &config_, stage_);
//     frame->set_text(title);
//     frame->resize(width, height);
//     stage_->append_child(frame);

//    return frame;
//}

// Button* UIManager::new_widget_as_button(const unicode &text, Px width, Px
// height, std::shared_ptr<WidgetStyle> shared_style) {
//     auto button = manager_->make_as<Button>(this, &config_, stage_,
//     shared_style); button->set_text(text); button->resize(width, height);
//     stage_->append_child(button);

//    return button;
//}

// TextEntry* UIManager::new_widget_as_text_entry(const unicode &text, Px width,
// Px height) {
//     auto label = (TextEntry*) &(*manager_->make_as<TextEntry>(this, &config_,
//     stage_)); label->set_text(text); label->resize(width, height);

//    stage_->append_child(label);

//    return label;
//}

// Label* UIManager::new_widget_as_label(const unicode &text, Px width, Px
// height) {
//     auto label = (Label*) &(*manager_->make_as<Label>(this, &config_,
//     stage_)); label->set_text(text); label->resize(width, height);

//    stage_->append_child(label);

//    return label;
//}

// Image* UIManager::new_widget_as_image(const TexturePtr& texture) {
//     auto image = (Image*) &(*manager_->make_as<Image>(this, &config_,
//     stage_)); image->set_texture(texture);

//    stage_->append_child(image);

//    return image;
//}

// Widget* UIManager::widget(StageNodeID widget_id) {
//     return manager_->get(widget_id);
// }

// ProgressBar* UIManager::new_widget_as_progress_bar(float min, float max,
// float value) {
//     auto pg = (ProgressBar*) &(*manager_->make_as<ProgressBar>(this,
//     &config_, stage_));

//    pg->set_range(min, max);
//    pg->set_value(value);

//    stage_->append_child(pg);

//    return pg;
//}

// void UIManager::destroy_widget(StageNodeID widget_id) {
//     if(!widget_id) {
//         return;
//     }

//    auto w = widget(widget_id);
//    if(!w) {
//        return;
//    }

//    // Release any presses on the widget
//    w->force_release();

//    // Queue for destruction
//    manager_->destroy(widget_id);
//}

// void UIManager::destroy_object(Widget* object) {
//     // Release any presses on the widget
//     object->force_release();

//    // Queue for destruction
//    manager_->destroy(object->id());
//}

// void UIManager::destroy_object_immediately(Widget* object) {
//     object->force_release();
//     manager_->destroy_immediately(object->id());
// }

void UIManager::on_mouse_down(const MouseEvent& evt) {
    if(!evt.is_touch_device) {
        queue_event(evt);
    }
}

void UIManager::on_mouse_up(const MouseEvent& evt) {
    if(!evt.is_touch_device) {
        queue_event(evt);
    }
}

void UIManager::on_touch_begin(const TouchEvent& evt) {
    queue_event(evt);
}

void UIManager::on_touch_end(const TouchEvent& evt) {
    queue_event(evt);
}

void UIManager::on_touch_move(const TouchEvent& evt) {
    queue_event(evt);
}

void UIManager::queue_event(const MouseEvent& e) {
    UIEvent evt(e);
    queued_events_.push_back(evt);
}

void UIManager::queue_event(const TouchEvent& e) {
    UIEvent evt(e);
    queued_events_.push_back(evt);
}

void UIManager::process_event_queue(const Camera* camera,
                                    const Viewport* viewport) const {

    if(queued_events_.empty()) {
        return;
    }

    auto queued_events = queued_events_; // Copy the queue

    for(auto& evt: queued_events) {
        switch(evt.type) {
            case UI_EVENT_TYPE_MOUSE: {
                auto widget = find_widget_at_window_coordinate(
                    camera, viewport, Vec2(evt.mouse.x, evt.mouse.y));
                if(widget) {
                    if(evt.mouse.type == MOUSE_EVENT_TYPE_BUTTON_DOWN) {
                        widget->fingerdown(evt.mouse.id.to_int8_t());
                    } else if(evt.mouse.type == MOUSE_EVENT_TYPE_BUTTON_UP) {
                        widget->fingerup(evt.mouse.id.to_int8_t());
                    }
                } else if(evt.mouse.type == MOUSE_EVENT_TYPE_MOTION) {
                    // If we just moved over the widget, and we weren't already
                    // on it then trigger a fingerenter
                    if(!widget->is_pressed_by_finger(
                           evt.mouse.id.to_int8_t())) {
                        widget->fingerenter(evt.mouse.id.to_int8_t());
                    }

                    // notify that there was a movement on the widget
                    widget->fingermove(evt.mouse.id.to_int8_t());
                }

                if(evt.mouse.type == MOUSE_EVENT_TYPE_MOTION ||
                   evt.mouse.type == MOUSE_EVENT_TYPE_BUTTON_UP) {
                    // Go through all the widgets, if one is being pressed and
                    // it's different than the one above, then trigger a
                    // fingerleave event
                    for(auto iter: find_child_widgets()) {
                        if(iter->is_pressed_by_finger(
                               evt.mouse.id.to_int8_t()) &&
                           iter != widget) {
                            iter->fingerleave(evt.mouse.id.to_int8_t());
                        }
                    }
                }
            } break;
            case UI_EVENT_TYPE_TOUCH: {
                auto widget = find_widget_at_window_coordinate(
                    camera, viewport,
                    Vec2(evt.touch.coord.x, evt.touch.coord.y));
                if(widget) {
                    if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_DOWN) {
                        widget->fingerdown(evt.touch.touch_id);
                    } else if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_UP) {
                        widget->fingerup(evt.touch.touch_id);
                    } else if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_MOVE) {
                        // If we just moved over the widget, and we weren't
                        // already on it then trigger a fingerenter
                        if(!widget->is_pressed_by_finger(evt.touch.touch_id)) {
                            widget->fingerenter(evt.touch.touch_id);
                        }

                        // notify that there was a movement on the widget
                        widget->fingermove(evt.touch.touch_id);
                    }
                }

                if(evt.touch.type == TOUCH_EVENT_TYPE_FINGER_MOVE ||
                   evt.touch.type == TOUCH_EVENT_TYPE_FINGER_UP) {
                    // Go through all the widgets, if one is being pressed and
                    // it's different than the one above, then trigger a
                    // fingerleave event
                    for(auto iter: find_child_widgets()) {
                        if(iter->is_pressed_by_finger(evt.touch.touch_id) &&
                           iter != widget) {
                            iter->fingerleave(evt.touch.touch_id);
                        }
                    }
                }

            } break;
            default:
                break;
        }
    }
}

void UIManager::clear_event_queue() {
    queued_events_.clear();
}

std::vector<Widget*> UIManager::find_child_widgets() const {
    auto nodes = find_descendents_by_types({
        STAGE_NODE_TYPE_WIDGET_BUTTON,
        STAGE_NODE_TYPE_WIDGET_FRAME,
        STAGE_NODE_TYPE_WIDGET_IMAGE,
        STAGE_NODE_TYPE_WIDGET_KEYBOARD,
        STAGE_NODE_TYPE_WIDGET_KEYBOARD_PANEL,
        STAGE_NODE_TYPE_WIDGET_LABEL,
        STAGE_NODE_TYPE_WIDGET_PROGRESS_BAR,
        STAGE_NODE_TYPE_WIDGET_TEXT_ENTRY,
    });

    std::vector<Widget*> widgets;
    widgets.reserve(nodes.size());
    for(auto& node: nodes) {
        widgets.push_back(static_cast<Widget*>(node));
    }

    return widgets;
}

WidgetPtr UIManager::find_widget_at_window_coordinate(
    const Camera* camera, const Viewport* viewport,
    const Vec2& window_coord) const {
    WidgetPtr result = nullptr;

    auto window = get_app()->window.get();

    std::list<Widget*> sorted_widgets;

    for(auto widget: find_child_widgets()) {
        if(!widget->is_visible()) {
            continue;
        }

        sorted_widgets.push_back(widget);
    }

    auto is_descendent = [](Widget* item, Widget* potential_parent) {
        StageNode* it = (StageNode*)item->parent();
        while(it && it != item->scene.get()) {
            if(it == (StageNode*)potential_parent) {
                return true;
            }
            it = (StageNode*)it->parent();
        }

        return false;
    };

    smlt::Plane cam_plane(camera->transform->forward(),
                          camera->transform->position());

    // Order widgets by distance to camera desc. This is hacky, if the two
    // widgets have approx the same distance, we check to see if one is a
    // descendent of the other, if so we sort descendents after ancestors
    sorted_widgets.sort(
        [cam_plane, &is_descendent](Widget* lhs, Widget* rhs) -> bool {
        float lhd = cam_plane.distance_to(lhs->transform->position());
        float rhd = cam_plane.distance_to(rhs->transform->position());

        if(almost_equal(lhd, rhd)) {
            if(is_descendent(lhs, rhs)) {
                // If lhs is a descendent of rhs, make sure it comes first in
                // the list
                return true;
            } else {
                // Not related, return false
                return false;
            }
        }

        return lhd < rhd;
    });

    for(auto widget: sorted_widgets) {
        auto aabb = widget->transformed_aabb();

        std::vector<Vec3> ss_points;

        for(auto& corner: aabb.corners()) {
            ss_points.push_back(
                camera->project_point(*window, *viewport, corner).value());
        }

        AABB ss_aabb(&ss_points[0], ss_points.size());

        // FIXME: Return the nearest if overlapping!
        if(ss_aabb.min().x <= window_coord.x &&
           ss_aabb.max().x >= window_coord.x &&
           ss_aabb.min().y <= window_coord.y &&
           ss_aabb.max().y >= window_coord.y) {
            result = widget;
            break;
        }
    };

    return result;
}

void UIManager::do_generate_renderables(batcher::RenderQueue* render_queue,
                                        const Camera* camera,
                                        const Viewport* viewport,
                                        const DetailLevel detail_level,
                                        Light** light,
                                        const std::size_t light_count) {

    _S_UNUSED(detail_level);
    _S_UNUSED(light);
    _S_UNUSED(light_count);

    /* Each time the scene is rendered with a camera and viewport, we need to
     * process any queued events so that (for example) we can interact with the
     * same widget rendered to different viewports */
    process_event_queue(camera, viewport);
}

} // namespace ui
} // namespace smlt
