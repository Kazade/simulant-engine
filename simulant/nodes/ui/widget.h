#include "../stage_node.h"
#include "../../generic/optional.h"
#include "../../generic/identifiable.h"

#pragma once

namespace smlt {
namespace ui {

struct Float4 {
    float top;
    float left;
    float bottom;
    float right;
};

enum OverflowType {
    OVERFLOW_TYPE_HIDDEN,
    OVERFLOW_TYPE_VISIBLE,
    OVERFLOW_TYPE_AUTO
};

enum TextAlignment {
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_CENTER,
    TEXT_ALIGNMENT_RIGHT
};

enum ResizeMode {
    RESIZE_MODE_FIXED,
    RESIZE_MODE_FIT_CONTENT
};

struct UIDim {
    float width = 0.0;
    float height = 0.0;
};

struct UIConfig {
    float default_width_ = 80;
    float default_height_ = 16;

    float font_size_ = 16;
    float line_height_ = 18;

    Colour foreground_colour_ = Colour::BLACK;
    Colour background_colour_ = Colour::WHITE;

    float scrollbar_width_ = 16;
    Colour scrollbar_background_colour_ = Colour::LIGHT_GREY;
    Colour scrollbar_foreground_colour_ = Colour::ALICE_BLUE;

    Colour button_background_color_ = Colour::ALICE_BLUE;
    Colour button_foreground_color_ = Colour::WHITE;
    Colour button_border_colour_ = Colour::WHITE;

    float button_border_width_ = 1;
    float button_border_radius_ = 3;

    OverflowType default_overflow_ = OVERFLOW_TYPE_HIDDEN;
    ResizeMode default_resize_mode_ = RESIZE_MODE_FIXED;
};

class UIManager;

class Widget:
    public StageNode,
    public generic::Identifiable<WidgetID> {

public:
    Widget(WidgetID id, UIManager* owner, UIConfig* defaults);

    virtual bool init();

    void resize(float width, float height);
    void set_width(float width);
    void set_height(float height);

    void set_text(const unicode& text) { set_text(text); }
    void set_border_width(float x);
    void set_border_width(float left, float right, float bottom, float top);
    void set_border_colour(const Colour& colour);
    void set_overflow(OverflowType type);
    void set_padding(float x);
    void set_padding(float left, float right, float bottom, float top);
    void set_resize_mode(ResizeMode resize_mode);

    void set_background_image(TextureID texture); // FIXME: Switch to TextureFrame when that's a thing

    float content_width() const; // Content area
    float content_height() const;

    float outer_width() const; // With border
    float outer_height() const;

    bool is_checked() const; // Widget dependent, returns false if widget has no concept of 'active'
    bool is_enabled() const; // Widget dependent, returns true if widget has no concept of 'disabled'
    bool is_hovered() const; // Widget dependent, returns false if widget has no concept of 'hovered'

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
    const AABB aabb() const;

private:
    UIManager* owner_;
    ActorID actor_;

    virtual ActorID construct_widget();
    virtual UIDim calc_content_dimensions();

    float width_;
    float height_;

    Float4 padding_;

    Float4 border_width_;
    Colour border_colour_;

    unicode text_;
    OverflowType overflow_;
    ResizeMode resize_mode_;

    std::unordered_map<std::string, smlt::any> properties_;

    virtual void on_size_changed() {}
};

}
}
