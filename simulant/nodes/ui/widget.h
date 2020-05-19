#pragma once

#include "../stage_node.h"
#include "../../generic/optional.h"
#include "../../generic/identifiable.h"
#include "../../generic/managed.h"
#include "../../generic/range_value.h"
#include "ui_manager.h"
#include "ui_config.h"

namespace smlt {
namespace ui {

class UIManager;

typedef sig::signal<void ()> WidgetPressedSignal;
typedef sig::signal<void ()> WidgetReleasedSignal; // Triggered on fingerup, but also on leave
typedef sig::signal<void ()> WidgetClickedSignal; // Triggered on fingerup only
typedef sig::signal<void ()> WidgetFocusedSignal;
typedef sig::signal<void ()> WidgetBlurredSignal;

class Widget:
    public TypedDestroyableObject<Widget, UIManager>,
    public ContainerNode,
    public generic::Identifiable<WidgetID>,
    public HasMutableRenderPriority {

    DEFINE_SIGNAL(WidgetPressedSignal, signal_pressed);
    DEFINE_SIGNAL(WidgetReleasedSignal, signal_released);
    DEFINE_SIGNAL(WidgetClickedSignal, signal_clicked);
    DEFINE_SIGNAL(WidgetFocusedSignal, signal_focused);
    DEFINE_SIGNAL(WidgetBlurredSignal, signal_blurred);
public:
    typedef std::shared_ptr<Widget> ptr;

    Widget(UIManager* owner, UIConfig* defaults);
    virtual ~Widget();

    virtual bool init() override;
    virtual void clean_up() override;

    void resize(int32_t width, int32_t height);
    void set_font(FontID font_id);

    /* Allow creating a double-linked list of widgets for focusing. There is no
     * global focused widget but there is only one focused widget in a chain
     */
    bool is_focused() const;
    void set_focus_previous(WidgetPtr previous_widget);
    void set_focus_next(WidgetPtr next_widget);
    void focus();
    void blur();
    void focus_next_in_chain(ChangeFocusBehaviour behaviour = FOCUS_THIS_IF_NONE_FOCUSED);
    void focus_previous_in_chain(ChangeFocusBehaviour behaviour = FOCUS_THIS_IF_NONE_FOCUSED);
    WidgetPtr first_in_focus_chain();
    WidgetPtr last_in_focus_chain();
    WidgetPtr focused_in_chain();

    // Manually trigger events
    void click();

    void set_text(const unicode& text);
    void set_border_width(float x);
    void set_border_colour(const Colour& colour);
    void set_overflow(OverflowType type);
    void set_padding(float x);
    void set_padding(float left, float right, float bottom, float top);
    virtual void set_resize_mode(ResizeMode resize_mode);

    ResizeMode resize_mode() const { return resize_mode_; }

    bool has_background_image() const { return bool(background_image_); }
    bool has_foreground_image() const { return bool(foreground_image_); }

    /** Set the background image, pass TextureID() to clear */
    void set_background_image(TexturePtr texture);

    /** Set the background to a region of its image. Coordinates are in texels */
    void set_background_image_source_rect(const Vec2& bottom_left, const Vec2& size);

    void set_background_colour(const Colour& colour);
    void set_foreground_colour(const Colour& colour);

    /** Set the foreground image, pass TextureID() to clear */
    void set_foreground_image(TexturePtr texture);

    /** Set the foreground to a region of its image. Coordinates are in texels */
    void set_foreground_image_source_rect(const Vec2& bottom_left, const Vec2& size);

    void set_text_colour(const Colour& colour);

    float requested_width() const { return requested_width_; }
    float requested_height() const { return requested_height_; }

    float content_width() const { return content_width_; } // Content area
    float content_height() const { return content_height_; }

    float outer_width() const { return content_width() + (border_width_ * 2); }
    float outer_height() const { return content_height() + (border_width_ * 2); }

    /*
    bool is_checked() const; // Widget dependent, returns false if widget has no concept of 'active'
    bool is_enabled() const; // Widget dependent, returns true if widget has no concept of 'disabled'
    bool is_hovered() const; // Widget dependent, returns false if widget has no concept of 'hovered'
    */

    void set_property(const std::string& name, float value);
    bool has_property(const std::string& name) const { return bool(properties_.count(name)); }

    template<typename T>
    smlt::optional<T> property(const std::string& name) const {
        if(!properties_.count(name)) {
            return smlt::optional<T>();
        }

        return smlt::optional<T>(smlt::any_cast<T>(properties_.at(name)));
    }

    const AABB& aabb() const;

    const unicode& text() const { return text_; }

    // Probably shouldn't use these directly (designed for UIManager)
    void fingerdown(uint32_t finger_id);
    void fingerup(uint32_t finger_id);
    void fingerenter(uint32_t finger_id);
    void fingermove(uint32_t finger_id);
    void fingerleave(uint32_t finger_id);
    bool is_pressed_by_finger(uint32_t finger_id);

    /* Releases all presses forcibly, firing signals */
    void force_release();

    void set_anchor_point(RangeValue<0, 1> x, RangeValue<0, 1> y);
    smlt::Vec2 anchor_point() const;

    void set_opacity(RangeValue<0, 1> alpha);

public:
    MaterialPtr material() const { return material_; }

private:
    void on_render_priority_changed(
        RenderPriority old_priority, RenderPriority new_priority
    ) override;

    bool initialized_ = false;
    UIManager* owner_ = nullptr;
    ActorPtr actor_ = nullptr;
    MeshPtr mesh_ = nullptr;
    FontPtr font_ = nullptr;
    MaterialPtr material_ = nullptr;

    int32_t requested_width_ = 0;
    int32_t requested_height_ = 0;

    int32_t content_width_ = 0;
    int32_t content_height_ = 0;

    UInt4 padding_ = {0, 0, 0, 0};

    float border_width_ = 1.0f;
    Colour border_colour_ = Colour::BLACK;

    unicode text_;
    OverflowType overflow_;
    ResizeMode resize_mode_ = RESIZE_MODE_FIT_CONTENT;

    struct ImageRect {
        Vec2 bottom_left;
        Vec2 size;
    };

    TexturePtr background_image_;
    ImageRect background_image_rect_;

    TexturePtr foreground_image_;
    ImageRect foreground_image_rect_;

    Colour background_colour_ = Colour::WHITE;
    Colour foreground_colour_ = Colour::NONE; //Transparent
    Colour text_colour_ = Colour::BLACK;
    float line_height_ = 16;

    std::unordered_map<std::string, smlt::any> properties_;

    virtual void on_size_changed();

    bool is_focused_ = false;
    WidgetPtr focus_next_ = nullptr;
    WidgetPtr focus_previous_ = nullptr;

    float opacity_ = 1.0f;

protected:
    struct WidgetBounds {
        smlt::Vec2 min;
        smlt::Vec2 max;

        float width() const { return max.x - min.x; }
        float height() const { return max.y - min.y; }
    };

    virtual WidgetBounds calculate_background_size(float content_width, float content_height) const;
    virtual WidgetBounds calculate_foreground_size(float content_width, float content_height) const;
    void apply_image_rect(SubMeshPtr submesh, TexturePtr image, ImageRect& rect);

    SubMeshPtr new_rectangle(const std::string& name, WidgetBounds bounds, const smlt::Colour& colour);
    void clear_mesh();

    bool is_initialized() const { return initialized_; }

    MeshPtr mesh() { return mesh_; }

    float background_depth_bias_ = 0.0001f;
    float foreground_depth_bias_ = 0.0002f;
    float text_depth_bias_ = 0.0004f;

    void render_text();

    std::set<uint32_t> fingers_down_;

    WidgetPtr focused_in_chain_or_this();

    /*
     * We regularly need to rebuild the text submesh. Wiping out vertex data
     * is cumbersome and slow, so instead we wipe the submesh indexes and add
     * them here, then used these indexes as necessary when rebuilding
     */
    std::set<uint16_t> available_indexes_;

    /* A normalized vector representing the relative
     * anchor position for movement (0, 0 == bottom left) */
    smlt::Vec2 anchor_point_;
    bool anchor_point_dirty_;

    void on_transformation_change_attempted();

    void rebuild();
};

}
}
