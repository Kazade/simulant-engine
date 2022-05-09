
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

template<typename PoolType, typename IDType, typename T, typename ...Subtypes>
class StageNodeManager;

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

typedef ::smlt::StageNodeManager<
    ::smlt::StageNodePool,
    WidgetID, Widget, Button, Label, ProgressBar, Image, Frame, Keyboard
> WidgetManager;

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

class UIManager:
    public EventListener {

    friend class Widget;

public:
    UIManager(Stage* stage, StageNodePool* pool, UIConfig config=UIConfig());
    virtual ~UIManager();

    Button* new_widget_as_button(
        const unicode& text,
        Px width=-1, Px height=-1,
        std::shared_ptr<WidgetStyle> shared_style=std::shared_ptr<WidgetStyle>()
    );

    Label* new_widget_as_label(const unicode& text, Px width=-1, Px height=-1);
    ProgressBar* new_widget_as_progress_bar(float min=.0f, float max=100.0f, float value=.0f);
    Image* new_widget_as_image(const TexturePtr& texture);
    Frame* new_widget_as_frame(const unicode& title, const Px& width=-1, const Px& height=-1);
    Keyboard* new_widget_as_keyboard(const KeyboardLayout& layout=KEYBOARD_LAYOUT_NUMERICAL);

    Widget* widget(WidgetID widget_id);

    void destroy_widget(WidgetID widget);

    Stage* stage() const { return stage_; }

    /* Implementation for TypedDestroyableObject (INTERNAL) */
    void destroy_object(Widget* object);
    void destroy_object_immediately(Widget* object);

    const UIConfig* config() const {
        return &config_;
    }
private:
    friend class ::smlt::Stage;

    Stage* stage_ = nullptr;

    std::shared_ptr<WidgetManager> manager_;
    UIConfig config_;

    void on_touch_begin(const TouchEvent &evt) override;
    void on_touch_end(const TouchEvent &evt) override;
    void on_touch_move(const TouchEvent &evt) override;

    void queue_event(const TouchEvent& evt);
    void process_event_queue(const Camera *camera, const Viewport& viewport) const;
    void clear_event_queue();

    std::queue<UIEvent> queued_events_;

    WidgetPtr find_widget_at_window_coordinate(const Camera *camera, const Viewport& viewport, const Vec2& window_coord) const;

    sig::connection frame_finished_connection_;
    sig::connection pre_render_connection_;

    FontPtr load_or_get_font(
        const std::string& family, const Px& size, const FontWeight &weight
    );

private:
    friend class ::smlt::Application;
    static FontPtr _load_or_get_font(VirtualFileSystem* vfs,
        AssetManager* assets, AssetManager* shared_assets,
        const std::string& family, const Px& size, const FontWeight &weight
    );

    MaterialPtr global_background_material_;
    MaterialPtr global_foreground_material_;
    MaterialPtr global_border_material_;

    MaterialPtr clone_global_background_material();
    MaterialPtr clone_global_foreground_material();
};

}
}
