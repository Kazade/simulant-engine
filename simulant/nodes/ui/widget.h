#pragma once

#include "../stage_node.h"
#include "../../generic/optional.h"
#include "../../generic/identifiable.h"
#include "../../generic/managed.h"
#include "../../generic/range_value.h"
#include "ui_config.h"

namespace smlt {
namespace ui {

enum WidgetLayerIndex {
    WIDGET_LAYER_INDEX_BORDER,
    WIDGET_LAYER_INDEX_BACKGROUND,
    WIDGET_LAYER_INDEX_FOREGROUND,
    WIDGET_LAYER_INDEX_TEXT,
};

class UIManager;

typedef sig::signal<void ()> WidgetPressedSignal;
typedef sig::signal<void ()> WidgetReleasedSignal; // Triggered on fingerup, but also on leave
typedef sig::signal<void ()> WidgetClickedSignal; // Triggered on fingerup only
typedef sig::signal<void ()> WidgetFocusedSignal;
typedef sig::signal<void ()> WidgetBlurredSignal;

struct ImageRect {
    UICoord bottom_left;
    UICoord size;
};

struct WidgetImpl {
    /* There are 4 layers: Border, background, foreground and text and
     * by default all are enabled. Setting any of the colours of these
     * layers to Colour::NONE will deactivate drawing of the layer
     * for performance reasons. We track that here */
    uint8_t active_layers_ = ~0;

    Px text_width_ = 0;
    Px text_height_ = 0;

    Px requested_width_ = -1;
    Px requested_height_ = -1;

    Px content_width_ = 0;
    Px content_height_ = 0;

    UInt4 padding_ = {0, 0, 0, 0};

    Px border_width_ = 1;
    PackedColour4444 border_colour_ = Colour::BLACK;

    TextAlignment text_alignment_ = TEXT_ALIGNMENT_CENTER;

    unicode text_;
    OverflowType overflow_;
    ResizeMode resize_mode_ = RESIZE_MODE_FIT_CONTENT;

    TexturePtr background_image_;
    ImageRect background_image_rect_;

    TexturePtr foreground_image_;
    ImageRect foreground_image_rect_;

    PackedColour4444 background_colour_ = Colour::WHITE;
    PackedColour4444 foreground_colour_ = Colour::NONE; //Transparent
    PackedColour4444 text_colour_ = Colour::BLACK;

    /* Line height is always stored as a Rem value, even if specified
     * as px. This is so changes in font size still work correctly */
    Rem line_height_ = Rem(1.1f);

    bool is_focused_ = false;
    WidgetPtr focus_next_ = nullptr;
    WidgetPtr focus_previous_ = nullptr;

    float opacity_ = 1.0f;

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

    std::set<uint8_t> fingers_down_;
};


/* Sizing:
 *
 * Widgets follow a similar model to the border-box model in CSS. Effectively:
 *
 * - If a dimension length has not been defined (e.g. requested_width_ == -1) then
 *   the dimension is calculated as the content size + padding + border.
 * - If a dimension length has been defined, then the content size is reduced to make
 *   room for the padding and border.
 *
 * Content area:
 *
 * - The size of the content area varies depending on widget, but for most widgets this
 *   is defined as the area that the text takes to render, unless a fixed size has been
 *   specified and then this would be the requested size without padding or border
 */

class Widget:
    public TypedDestroyableObject<Widget, UIManager>,
    public ContainerNode,
    public generic::Identifiable<WidgetID>,
    public HasMutableRenderPriority,
    public ChainNameable<Widget>  {

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

    void resize(Rem width, Px height);
    void resize(Px width, Rem height);
    void resize(Rem width, Rem height);
    void resize(Px width, Px height);

    void set_font(const std::string& family=DEFAULT_FONT_FAMILY, Rem size=Rem(1.0f), FontWeight weight=FONT_WEIGHT_NORMAL);
    void set_font(const std::string& family=DEFAULT_FONT_FAMILY, Px size=DEFAULT_FONT_SIZE, FontWeight weight=FONT_WEIGHT_NORMAL);

    /* Allow creating a double-linked list of widgets for focusing. There is no
     * global focused widget but there is only one focused widget in a chain
     */
    bool is_focused() const;
    void set_focus_previous(WidgetPtr previous_widget);
    void set_focus_next(WidgetPtr next_widget);
    void focus();
    void blur();

    WidgetPtr next_in_focus_chain() const;
    WidgetPtr previous_in_focus_chain() const;
    void focus_next_in_chain(ChangeFocusBehaviour behaviour = FOCUS_THIS_IF_NONE_FOCUSED);
    void focus_previous_in_chain(ChangeFocusBehaviour behaviour = FOCUS_THIS_IF_NONE_FOCUSED);
    WidgetPtr first_in_focus_chain();
    WidgetPtr last_in_focus_chain();
    WidgetPtr focused_in_chain();

    // Manually trigger events
    void click();

    void set_text(const unicode& text);

    void set_text_alignment(TextAlignment alignment);
    TextAlignment text_alignment() const;

    void set_border_width(Px x);
    Px border_width() const;

    void set_border_colour(const Colour& colour);
    void set_overflow(OverflowType type);

    void set_padding(Px x);
    void set_padding(Px left, Px right, Px bottom, Px top);
    UInt4 padding() const;

    virtual bool set_resize_mode(ResizeMode resize_mode);

    ResizeMode resize_mode() const;

    bool has_background_image() const;

    bool has_foreground_image() const;

    /** Set the background image, pass TextureID() to clear */
    void set_background_image(TexturePtr texture);

    /** Set the background to a region of its image. Coordinates are in texels */
    void set_background_image_source_rect(const UICoord& bottom_left, const UICoord& size);

    void set_background_colour(const Colour& colour);
    void set_foreground_colour(const Colour& colour);

    /** Set the foreground image, pass TextureID() to clear */
    void set_foreground_image(TexturePtr texture);

    /** Set the foreground to a region of its image. Coordinates are in texels */
    void set_foreground_image_source_rect(const UICoord& bottom_left, const UICoord& size);

    void set_text_colour(const Colour& colour);

    Px requested_width() const;
    Px requested_height() const;

    Px content_width() const;
    Px content_height() const;

    /** This returns the outside width of the widget, this will be the same as the
     * requested_width if it was specified, else it will be the width of the content
     * area plus padding and border */
    Px outer_width() const;
    Px outer_height() const;

    /*
    bool is_checked() const; // Widget dependent, returns false if widget has no concept of 'active'
    bool is_enabled() const; // Widget dependent, returns true if widget has no concept of 'disabled'
    bool is_hovered() const; // Widget dependent, returns false if widget has no concept of 'hovered'
    */


    const AABB& aabb() const override;

    const unicode& text() const {
        return pimpl_->text_;
    }

    // Probably shouldn't use these directly (designed for UIManager)
    void fingerdown(uint8_t finger_id);
    void fingerup(uint8_t finger_id);
    void fingerenter(uint8_t finger_id);
    void fingermove(uint8_t finger_id);
    void fingerleave(uint8_t finger_id);
    bool is_pressed_by_finger(uint8_t finger_id);

    /* Releases all presses forcibly, firing signals */
    void force_release();

    void set_anchor_point(RangeValue<0, 1> x, RangeValue<0, 1> y);
    smlt::Vec2 anchor_point() const;

    void set_opacity(RangeValue<0, 1> alpha);

    Px line_height() const;
public:
    MaterialPtr border_material() const { return materials_[0]; }
    MaterialPtr background_material() const { return materials_[1]; }
    MaterialPtr foreground_material() const { return materials_[2]; }

private:
    void on_render_priority_changed(
        RenderPriority old_priority, RenderPriority new_priority
    ) override;

    bool initialized_ = false;

    ActorPtr actor_ = nullptr;
    MeshPtr mesh_ = nullptr;

    MaterialPtr materials_[3] = {nullptr, nullptr, nullptr};

protected:
    UIManager* owner_ = nullptr;
    FontPtr font_ = nullptr;

    WidgetImpl* pimpl_ = nullptr;

    virtual void on_size_changed();

    /* Only called on construction, it just makes sure that
     * active_layers_ is in sync with whatever the default layer
     * colours are. If we change a layer colour we manually alter
     * the active_layers_ flag from that point on */
    void _recalc_active_layers();

    bool border_active() const;
    bool background_active() const;
    bool foreground_active() const;

    struct WidgetBounds {
        UICoord min;
        UICoord max;

        Px width() const { return max.x - min.x; }
        Px height() const { return max.y - min.y; }

        bool has_non_zero_area() const {
            Px w = width();
            Px h = height();
            return std::abs(w.value) > 0 && std::abs(h.value) > 0;
        }
    };

    virtual WidgetBounds calculate_background_size(const UIDim& content_dimensions) const;
    virtual WidgetBounds calculate_foreground_size(const UIDim& content_dimensions) const;

    virtual UIDim calculate_content_dimensions(Px text_width, Px text_height);


    void apply_image_rect(SubMeshPtr submesh, TexturePtr image, ImageRect& rect);

    SubMeshPtr new_rectangle(const std::string& name, WidgetBounds bounds, const smlt::Colour& colour);
    void clear_mesh();

    bool is_initialized() const { return initialized_; }

    MeshPtr mesh() { return mesh_; }

    void render_text();

    WidgetPtr focused_in_chain_or_this();

    void on_transformation_change_attempted() override;

    void rebuild();
    void set_font(FontPtr font_id);

    virtual void prepare_build() {}
    virtual void finalize_build() {}
};

}
}
