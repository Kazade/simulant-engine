
#pragma once

#include "../../event_listener.h"
#include "../../generic/containers/polylist.h"
#include "../../types.h"
#include "../stage_node.h"
#include "../stage_node_pool.h"
#include "keyboard.h"
#include "ui_config.h"
#include <map>
#include <queue>

namespace smlt {

class Application;
class VirtualFileSystem;
class SharedAssetManager;

namespace ui {

class Widget;
class Button;
class Label;
class ProgressBar;
class Image;
class Frame;
class Keyboard;
class TextEntry;

enum UIEventType {
    UI_EVENT_TYPE_TOUCH,
    UI_EVENT_TYPE_MOUSE,
};

struct UIEvent {
    UIEvent(const MouseEvent& evt) :
        type(UI_EVENT_TYPE_MOUSE), mouse(evt) {}

    UIEvent(const TouchEvent& evt) :
        type(UI_EVENT_TYPE_TOUCH), touch(evt) {}

    UIEventType type;
    union {
        TouchEvent touch;
        MouseEvent mouse;
    };
};

class UIManager: public EventListener, public StageNode {

    friend class Widget;

public:
    S_DEFINE_STAGE_NODE_META(STAGE_NODE_TYPE_UI_MANAGER);

    UIManager(Scene* owner, UIConfig config = UIConfig());
    virtual ~UIManager();

    const UIConfig* config() const {
        return &config_;
    }

private:
    UIConfig config_;

    bool on_create(const ConstructionArgs& params) override;

    void on_mouse_down(const MouseEvent& evt) override;
    void on_mouse_up(const MouseEvent& evt) override;

    void on_touch_begin(const TouchEvent& evt) override;
    void on_touch_end(const TouchEvent& evt) override;
    void on_touch_move(const TouchEvent& evt) override;

    void queue_event(const TouchEvent& evt);
    void queue_event(const MouseEvent& evt);
    void process_event_queue(const Camera* camera,
                             const Viewport* viewport) const;
    void clear_event_queue();

    std::vector<UIEvent> queued_events_;

    WidgetPtr find_widget_at_window_coordinate(const Camera* camera,
                                               const Viewport* viewport,
                                               const Vec2& window_coord) const;

    sig::connection frame_finished_connection_;
    sig::connection pre_render_connection_;

    virtual void
        do_generate_renderables(batcher::RenderQueue* render_queue,
                                const Camera* camera, const Viewport* viewport,
                                const DetailLevel detail_level) override;

private:
    friend class ::smlt::Application;

    std::vector<Widget*> find_child_widgets() const;
};

} // namespace ui
} // namespace smlt
