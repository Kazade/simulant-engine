#include "../stage_node.h"
#include "../../generic/optional.h"
#include "../../generic/identifiable.h"

#pragma once

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
    ResizeMode button_resize_mode_ = RESIZE_MODE_FIT_CONTENT;
    ResizeMode progress_bar_resize_mode_ = RESIZE_MODE_FIXED;

    float scrollbar_width_ = 16;
    Colour scrollbar_background_colour_ = Colour::LIGHT_GREY;
    Colour scrollbar_foreground_colour_ = Colour::ALICE_BLUE;

    float button_height_ = 36;
    float button_width_ = 0; // Fit content

    Float4 button_padding_ = { 30, 30, 0, 0 };
    Colour button_background_color_ = Colour::ALICE_BLUE;
    Colour button_foreground_color_ = Colour::WHITE;
    Colour button_border_colour_ = Colour::WHITE;

    float button_border_width_ = 1;
    float button_border_radius_ = 3;

    Colour progress_bar_foreground_colour_ = Colour::DODGER_BLUE;
    Colour progress_bar_background_colour_ = Colour::WHITE;
    Colour progress_bar_border_colour_ = Colour::DODGER_BLUE;
    float progress_bar_border_width_ = 1;

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
    void set_font(FontID font);

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

    const unicode& text() const { return text_; }
private:
    bool initialized_ = false;
    UIManager* owner_;
    ActorID actor_;
    MeshPtr mesh_;
    FontPtr font_;
    MaterialPtr material_;

    virtual MeshID construct_widget(float requested_width, float requested_height);
    virtual UIDim calc_content_dimensions();

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
protected:
    bool is_initialized() const { return initialized_; }

    MeshPtr mesh() { return mesh_; }

    float background_depth_bias_ = 0.00001f;
    float foreground_depth_bias_ = 0.00002f;
    float text_depth_bias_ = 0.00003f;

    void resize_foreground(MeshPtr mesh, float width, float height, float xoffset, float yoffset);
    void render_text(MeshPtr mesh, const std::string& submesh_name, const unicode& text, float width);
};

}
}
