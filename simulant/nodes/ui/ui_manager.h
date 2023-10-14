
#pragma once

#include <map>
#include <queue>
#include "../../types.h"
#include "../../event_listener.h"
#include "ui_config.h"
#include "../../generic/containers/polylist.h"
#include "../stage_node.h"
#include "../stage_node_pool.h"
#include "keyboard.h"

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
    UI_EVENT_TYPE_TOUCH
};

struct UIEvent {
    UIEvent(const TouchEvent& evt):
        type(UI_EVENT_TYPE_TOUCH),
        touch(evt) {}

    UIEventType type;
    union {
        TouchEvent touch;
    };
};


struct UIManagerParams {
    UIConfig config;
};



class UIManager:
    public EventListener,
    public StageNode {

    friend class Widget;

public:
    UIManager(Scene* owner, UIConfig config=UIConfig());
    virtual ~UIManager();

    const UIConfig* config() const {
        return &config_;
    }

private:
    UIConfig config_;

    void on_touch_begin(const TouchEvent &evt) override;
    void on_touch_end(const TouchEvent &evt) override;
    void on_touch_move(const TouchEvent &evt) override;

    void queue_event(const TouchEvent& evt);
    void process_event_queue(
        const Camera* camera,
        const Viewport* viewport
    ) const;
    void clear_event_queue();

    std::vector<UIEvent> queued_events_;

    WidgetPtr find_widget_at_window_coordinate(const Camera *camera, const Viewport *viewport, const Vec2& window_coord) const;

    sig::connection frame_finished_connection_;
    sig::connection pre_render_connection_;

    virtual void do_generate_renderables(batcher::RenderQueue* render_queue,
        const Camera*camera, const Viewport* viewport,
        const DetailLevel detail_level
    ) override;

private:
    friend class ::smlt::Application;

    std::vector<Widget*> find_child_widgets() const;
};

}

template<>
struct stage_node_traits<ui::UIManager> {
    typedef ui::UIManagerParams params_type;
    const static StageNodeType node_type = STAGE_NODE_TYPE_UI_MANAGER;
};

}
