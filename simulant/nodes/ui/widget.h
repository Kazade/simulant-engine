#pragma once

#include "../stage_node.h"
#include "../../generic/optional.h"
#include "../../generic/identifiable.h"
#include "../../generic/managed.h"

namespace smlt {
namespace ui {

struct Float4 {
    float left;
    float right;
    float bottom;
    float top;
};

enum OverflowType {
    OVERFLOW_TYPE_HIDDEN,
    OVERFLOW_TYPE_VISIBLE,
    OVERFLOW_TYPE_AUTO
};

/*
enum TextAlignment {
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_CENTER,
    TEXT_ALIGNMENT_RIGHT
};*/

enum ResizeMode {
    RESIZE_MODE_FIXED, // Clips / scrolls text
    RESIZE_MODE_FIXED_WIDTH, // Will expand vertically with text, text is word-wrapped
    RESIZE_MODE_FIXED_HEIGHT, // Will expand horizontally with text, text is not wrapped
    RESIZE_MODE_FIT_CONTENT // Will fit the text, newlines affect the height
};

enum ChangeFocusBehaviour {
    FOCUS_THIS_IF_NONE_FOCUSED = 0x1,
    FOCUS_NONE_IF_NONE_FOCUSED = 0x2
};

struct UIDim {
    float width = 0.0;
    float height = 0.0;
};

struct UIConfig {    
    float font_size_ = 16;
    float line_height_ = 18;

    Colour foreground_colour_ = Colour::BLACK;
    Colour background_colour_ = Colour::WHITE;

    ResizeMode label_resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    ResizeMode button_resize_mode_ = RESIZE_MODE_FIXED_HEIGHT;
    ResizeMode progress_bar_resize_mode_ = RESIZE_MODE_FIXED;

    float scrollbar_width_ = 16;
    Colour scrollbar_background_colour_ = Colour::LIGHT_GREY;
    Colour scrollbar_foreground_colour_ = Colour::ALICE_BLUE;

    float button_height_ = 36;
    float button_width_ = 0; // Fit content

    Float4 label_padding_ = { 5, 5, 5, 5 };
    Colour label_background_colour_ = Colour::NONE;
    Colour label_foreground_colour_ = Colour::NONE;
    Colour label_border_colour_ = Colour::NONE;
    Colour label_text_colour_ = Colour::DODGER_BLUE;

    Float4 button_padding_ = { 30, 30, 20, 20 };
    Colour button_background_colour_ = Colour::DODGER_BLUE;
    Colour button_foreground_colour_ = Colour::NONE;
    Colour button_text_colour_ = Colour::WHITE;
    Colour button_border_colour_ = Colour::NONE;

    float button_border_width_ = 0;
    float button_border_radius_ = 3;

    Colour progress_bar_foreground_colour_ = Colour::DODGER_BLUE;
    Colour progress_bar_background_colour_ = Colour::WHITE;
    Colour progress_bar_border_colour_ = Colour::DODGER_BLUE;
    float progress_bar_border_width_ = 1;

    OverflowType default_overflow_ = OVERFLOW_TYPE_HIDDEN;
    ResizeMode default_resize_mode_ = RESIZE_MODE_FIXED;
};

class UIManager;

typedef sig::signal<void ()> WidgetPressedSignal;
typedef sig::signal<void ()> WidgetReleasedSignal; // Triggered on fingerup, but also on leave
typedef sig::signal<void ()> WidgetClickedSignal; // Triggered on fingerup only
typedef sig::signal<void ()> WidgetFocusedSignal;
typedef sig::signal<void ()> WidgetBlurredSignal;

class Widget:
    public StageNode,
    public generic::Identifiable<WidgetID> {

    DEFINE_SIGNAL(WidgetPressedSignal, signal_pressed);
    DEFINE_SIGNAL(WidgetReleasedSignal, signal_released);
    DEFINE_SIGNAL(WidgetClickedSignal, signal_clicked);
    DEFINE_SIGNAL(WidgetFocusedSignal, signal_focused);
    DEFINE_SIGNAL(WidgetBlurredSignal, signal_blurred);
public:
    typedef std::shared_ptr<Widget> ptr;

    Widget(WidgetID id, UIManager* owner, UIConfig* defaults);
    virtual ~Widget();

    virtual bool init();
    virtual void cleanup();

    void resize(float width, float height);
    void set_width(float width);
    void set_height(float height);
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
    void set_resize_mode(ResizeMode resize_mode);

    void set_background_image(TextureID texture); // FIXME: Switch to TextureFrame when that's a thing
    void set_background_colour(const Colour& colour);

    void set_foreground_colour(const Colour& colour);
    void set_foreground_image(TextureID texture);

    void set_text_colour(const Colour& colour);

    float requested_width() const { return width_; }
    float requested_height() const { return height_; }

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
    smlt::optional<T> property(const std::string& name) {
        if(!properties_.count(name)) {
            return smlt::optional<T>();
        }

        return smlt::optional<T>(smlt::any_cast<T>(properties_.at(name)));
    }

    void ask_owner_for_destruction();
    const AABB& aabb() const;

    const unicode& text() const { return text_; }

    // Probably shouldn't use these directly (designed for UIManager)
    void fingerdown(uint32_t finger_id);
    void fingerup(uint32_t finger_id);
    void fingerenter(uint32_t finger_id);
    void fingermove(uint32_t finger_id);
    void fingerleave(uint32_t finger_id);
    bool is_pressed_by_finger(uint32_t finger_id);

private:
    bool initialized_ = false;
    UIManager* owner_ = nullptr;
    ActorPtr actor_ = nullptr;
    MeshPtr mesh_ = nullptr;
    FontPtr font_ = nullptr;
    MaterialPtr material_ = nullptr;

    virtual MeshPtr construct_widget(float requested_width, float requested_height);

    float width_ = .0f;
    float height_ = .0f;

    float content_width_ = .0f;
    float content_height_ = .0f;

    Float4 padding_ = {0, 0, 0, 0};

    float border_width_ = 1.0f;
    Colour border_colour_ = Colour::BLACK;

    unicode text_;
    OverflowType overflow_;
    ResizeMode resize_mode_ = RESIZE_MODE_FIXED_WIDTH;

    Colour background_colour_ = Colour::WHITE;
    Colour foreground_colour_ = Colour::NONE; //Transparent
    Colour text_colour_ = Colour::BLACK;
    float line_height_ = 16;

    std::unordered_map<std::string, smlt::any> properties_;

    virtual void on_size_changed();
    void rebuild();

    bool is_focused_ = false;
    WidgetPtr focus_next_ = nullptr;
    WidgetPtr focus_previous_ = nullptr;

protected:
    bool is_initialized() const { return initialized_; }

    MeshPtr mesh() { return mesh_; }

    float background_depth_bias_ = 0.0001f;
    float foreground_depth_bias_ = 0.0002f;
    float text_depth_bias_ = 0.0004f;

    void resize_foreground(MeshPtr mesh, float width, float height, float xoffset, float yoffset);
    void render_text(MeshPtr mesh, const std::string& submesh_name, const unicode& text, float width, float xoffset=0, float yoffset=0);

    std::set<uint32_t> fingers_down_;

    WidgetPtr focused_in_chain_or_this();

    /*
     * We regularly need to rebuild the text submesh. Wiping out vertex data
     * is cumbersome and slow, so instead we wipe the submesh indexes and add
     * them here, then used these indexes as necessary when rebuilding
     */
    std::set<uint16_t> available_indexes_;
};

}
}
