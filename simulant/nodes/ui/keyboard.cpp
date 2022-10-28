#include <vector>
#include "ui_manager.h"
#include "keyboard.h"
#include "button.h"
#include "frame.h"

#include "../../window.h"
#include "../../stage.h"
#include "../../application.h"
#include "../../event_listener.h"

namespace smlt {
namespace ui {


class KeyboardListener : public EventListener {
public:
    KeyboardListener(Keyboard* keyboard):
        keyboard_(keyboard) {
        smlt::get_app()->window->register_event_listener(this);
    }

    ~KeyboardListener() {
        smlt::get_app()->window->unregister_event_listener(this);
    }

    void on_key_up(const KeyEvent& evt) {        

    }

private:
    Keyboard* keyboard_ = nullptr;
};

const uint8_t SPACE_ICON [] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[16 * 16 * 2 + 1];
} ENTER_ICON = {
  16, 16, 2,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\242\020\323\234\353Z\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\242\020\000\000\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\060\204\333\336(B\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\060\204\276\367\272\326\246\061\000\000\000\000e)\377\377\363\234\000\000\000\000\000\000\000"
  "\000\000\000\000\000\060\204\276\367\272\326\307\071\000\000\000\000\000\000e)\377\377\363\234\000"
  "\000\000\000\000\000\000\000\000\000\060\204\276\367\377\377\333\336\272\326\272\326\272\326"
  "\272\326\272\326\377\377\363\234\000\000\000\000\000\000\000\000\000\000\060\204\276\367\377\377"
  "\333\336\272\326\272\326\272\326\272\326\272\326\272\326\020\204\000\000\000\000\000"
  "\000\000\000\000\000\000\000\060\204\276\367\272\326\307\071\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\060\204\276\367\272\326\246\061\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\060\204\333\336(B\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\242\020\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
};

static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[16 * 16 * 2 + 1];
} CASE_TOGGLE_ICON = {
  16, 16, 2,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\252R\252R\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\252R\034\347\034\347\252R\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\252"
  "R\373\336\377\377\377\377\373\336\252R\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\252R\034\347}\357}\357}\357}\357\034\347\252R\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\252R\373\336\236\367\256s\373\336\373\336\256s\236\367\373\336\252R\000"
  "\000\000\000\000\000\000\000\000\000\252R\034\347\236\367\317{\000\000\373\336\373\336\000\000\317{"
  "\236\367\034\347\252R\000\000\000\000\000\000\000\000\252R\272\326\256s\000\000\000\000\373\336\373"
  "\336\000\000\000\000\317{\272\326\252R\000\000\000\000\000\000\000\000\000\000E)\000\000\000\000\000\000\373\336"
  "\373\336\000\000\000\000\000\000e)\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\373\336\373"
  "\336\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\373\336\373"
  "\336\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\034\347\034\347"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\317{\317{\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000",
};

static const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[16 * 16 * 2 + 1];
} BACKSPACE_ICON = {
  16, 16, 2,
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\262"
  "\224\276\367\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\070\306\000\000\000\000,c}\357\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\000\000\343"
  "\030y\316\377\377\377\377\377\377\276\367\337\377\377\377\377\377\377\377"
  "}\357\377\377\377\377\377\377\377\377\000\000\064\245\377\377\377\377\377\377"
  "\377\377\262\224\222\224\337\377\377\377<\347,c\070\306\377\377\377\377\377"
  "\377,c\236\367\377\377\377\377\377\377\377\377<\347\014c\222\224\373\336\313"
  "Z\222\224\337\377\377\377\377\377\377\377\232\326\377\377\377\377\377\377"
  "\377\377\377\377\377\377<\347\313Z\246\061\222\224\276\367\377\377\377\377"
  "\377\377\377\377\232\326\377\377\377\377\377\377\377\377\377\377\377\377"
  "<\347\313Z\246\061\222\224\276\367\377\377\377\377\377\377\377\377,c\236\367"
  "\377\377\377\377\377\377\377\377<\347\014c\222\224\373\336\313Z\222\224\337"
  "\377\377\377\377\377\377\377\000\000\064\245\377\377\377\377\377\377\377\377\262"
  "\224\222\224\337\377\377\377<\347,c\070\306\377\377\377\377\377\377\000\000\343"
  "\030y\316\377\377\377\377\377\377\276\367\337\377\377\377\377\377\377\377"
  "}\357\377\377\377\377\377\377\377\377\000\000\000\000,c}\357\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\000\000\000\000\000\000\262\224\276\367\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\070\306\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
};

class KeyboardPanel:
    public Widget,
    public RefCounted<KeyboardPanel> {

    friend class Keyboard;

public:
    KeyboardPanel(UIConfig* config, Stage* stage):
        Widget(nullptr, config, stage) {}

    bool init() override {
        auto ret = Widget::init();
        if(!ret) {
            return ret;
        }

        auto space_tex = stage->assets->new_texture(16, 8, TEXTURE_FORMAT_R_1UB_8);
        space_tex->set_data(SPACE_ICON, 16 * 8);
        space_tex->flip_vertically();
        space_tex->convert(
            TEXTURE_FORMAT_RGBA_4UB_8888,
            {TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED, TEXTURE_CHANNEL_RED}
        );
        space_tex->flush();

        space_material_ = stage->assets->new_material_from_texture(space_tex);

        return true;
    }

private:
    MaterialPtr backspace_material_;
    MaterialPtr space_material_;
    SubMeshPtr space_submesh_ = nullptr;

    struct KeyInfo {
        int x = -1;  /* X/Y in positions, not pixels */
        int y = -1;

        UICoord center;  /*  Center position of the key */
        WidgetBounds bounds; /* Bounds of the key */
        char16_t displayed_character = 0;  /* Displayed char if any */

        /* Index of the first of the 4 vertices that make up the background
         * of this key */
        int32_t first_vertex_index = -1;
        bool is_focused = false;
    };

    std::vector<KeyInfo> keys_;

    Px key_width() const {
        return line_height() * 3;
    }

    Px key_height() const {
        return line_height() * 2;
    }

    Px key_padding() const {
        return Px(2);
    }

    int columns() const {
        return 12;
    }

    int rows() const {
        return 5;
    }

    KeyInfo* find_key(int x, int y) {
        for(auto& key: keys_) {
            if(key.x == x && key.y == y) {
                return &key;
            }
        }

        return nullptr;
    }

    KeyInfo* focused_key_ = nullptr;

    void focus_key(int x, int y) {
        auto fg_color = calc_foreground_colour();
        auto highlight_colour = theme_->highlight_colour_;

        auto key = find_key(x, y);
        if(key) {
            if(focused_key_) {
                mesh_->vertex_data->move_to(focused_key_->first_vertex_index);
                for(int i = 0; i < 4; ++i) {
                    mesh_->vertex_data->diffuse(fg_color);
                    mesh_->vertex_data->move_next();
                }
            }

            mesh_->vertex_data->move_to(key->first_vertex_index);
            for(int i = 0; i < 4; ++i) {
                mesh_->vertex_data->diffuse(highlight_colour);
                mesh_->vertex_data->move_next();
            }

            mesh_->vertex_data->done();
            focused_key_ = key;
        }
    }

    void finalize_build() override {
        if(!space_submesh_) {
            space_submesh_ = mesh_->new_submesh("space", space_material_, MESH_ARRANGEMENT_TRIANGLE_STRIP);
        }

        /* Space key */
        auto key = find_key(4, 3); /* Forth row, 4th button */
        if(key) {
            new_rectangle("space", key->bounds, smlt::Colour::WHITE);
        }

        if(!focused_key_) {
            focus_key(0, 0);
        }
    }

    void render_text() override {
        auto c = style_->text_colour_;
        c.set_alpha(style_->opacity_);

        auto sm = mesh_->find_submesh("text");
        assert(sm);

        /* Make sure the font material is up to date! */
        sm->set_material(font_->material());

        const char16_t row0 [12] = {
            '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-'
        };

        const char16_t row1 [12] = {
            'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '/'
        };

        const char16_t row2 [12] = {
            'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\''
        };

        const char16_t row3 [12] = {
            'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '!', '?'
        };

        int y = 0;
        for(auto& row: {row0, row1, row2, row3}) {
            int x = 0;
            for(int i = 0; i < 12; ++i) {
                auto ch = row[i];

                Px ch_width = font_->character_width(ch);
                Px ch_height = font_->character_height(ch);
                auto min_max = font_->texture_coordinates_for_character(ch);

                auto info = find_key(x, y);
                if(info) {
                    WidgetBounds bounds;

                    bounds.min.x = info->center.x - (ch_width / 2);
                    bounds.max.x = info->center.x + (ch_width / 2);
                    bounds.min.y = info->center.y - (ch_height / 2);
                    bounds.max.y = info->center.y + (ch_height / 2);

                    const smlt::Vec2 uvs [] = {
                        smlt::Vec2(min_max.first.x, min_max.second.y),
                        smlt::Vec2(min_max.second.x, min_max.second.y),
                        smlt::Vec2(min_max.first.x, min_max.first.y),
                        smlt::Vec2(min_max.second.x, min_max.first.y)
                    };

                    sm = new_rectangle("text", bounds, c, uvs);

                    info->displayed_character = ch;
                }
                ++x;
            }

            ++y;
        }
    }

    Colour calc_foreground_colour() const {
        auto colour = style_->foreground_colour_;
        colour.set_alpha(colour.af() * style_->opacity_);
        return colour;
    }

    void render_foreground(const WidgetBounds& bounds) override {
        _S_UNUSED(bounds);

        auto colour = calc_foreground_colour();

        WidgetBounds key_bounds;

        int focused_x = -1, focused_y = -1;

        if(focused_key_) {
            focused_x = focused_key_->x;
            focused_y = focused_key_->y;
        }

        keys_.clear();
        focused_key_ = nullptr;

        /* We build the key array here, this doesn't tell us what key
         * is which, but sets the positions of the keys */

        for(int y = 0; y < rows(); ++y) {
            for(int x = 0; x < columns(); ++x) {
                if(x < columns() - 1) {
                    key_bounds.min.x = (key_width() * x);
                    key_bounds.min.y = (key_height() * y);

                    key_bounds.min.x += key_padding() * (x + 1);
                    key_bounds.min.y += key_padding() * (y + 1);

                    key_bounds.min.x -= (bounds.width() / 2);
                    key_bounds.min.y -= (bounds.height() / 2);

                    if(y == 0 && x == 3) {
                        /* Space bar */
                        key_bounds.max.x = key_bounds.min.x + ((key_width() + key_padding()) * (columns() - 5));
                        key_bounds.max.y = key_bounds.min.y + key_height();
                    } else {
                        /* All other keys */
                        key_bounds.max.x = key_bounds.min.x + key_width();
                        key_bounds.max.y = key_bounds.min.y + key_height();
                    }
                } else {
                    /* Right-hand action buttons! */
                    key_bounds.min.x = (key_width() * x);
                    key_bounds.min.y = (key_height() * y);

                    key_bounds.min.x += key_padding() * (x + 1);
                    key_bounds.min.y += key_padding() * (y + 1);

                    key_bounds.min.x -= (bounds.width() / 2);
                    key_bounds.min.y -= (bounds.height() / 2);

                    if(y == 4) {
                        key_bounds.max.x = key_bounds.min.x + key_width();
                        key_bounds.max.y = key_bounds.min.y + key_height();
                    } else if(y == 0) {
                        key_bounds.max.x = key_bounds.min.x + key_width();
                        key_bounds.max.y = key_bounds.min.y + (key_height() * 2) + key_padding();
                    } else if(y == 2) {
                        key_bounds.max.x = key_bounds.min.x + key_width();
                        key_bounds.max.y = key_bounds.min.y + (key_height() * 2) + key_padding();
                    } else {
                        continue;
                    }
                }

                KeyInfo new_key;
                new_key.x = x;
                new_key.y = rows() - y - 1;
                new_key.bounds = key_bounds;
                new_key.center = key_bounds.min;
                new_key.center.x += (key_bounds.width() / 2);
                new_key.center.y += (key_bounds.height() / 2);
                new_key.first_vertex_index = mesh_->vertex_data->count();

                keys_.push_back(new_key);

                new_rectangle("foreground", key_bounds, colour);
            }
        }

        if(focused_x > -1 && focused_y > -1) {
            focus_key(focused_x, focused_y);
        }
    }

    /* The content dimensions are nothing to do with the text. We don't do anything with
     * text set on a keyboard widget. Instead the dimensions are defined by the number of
     * keys and the font size etc. */
    UIDim calculate_content_dimensions(Px text_width, Px text_height) override {
        _S_UNUSED(text_width);
        _S_UNUSED(text_height);

        Px height = (key_height() * rows()) + (key_padding() * (rows() + 1));
        Px width = (key_width() * columns()) + (key_padding() * (columns() + 1));
        return UIDim(width, height);
    }
};

Keyboard::Keyboard(UIManager *owner, UIConfig *config, Stage* stage, KeyboardLayout layout):
    Widget(owner, config, stage),
    layout_(layout) {

    main_frame_ = owner->new_widget_as_frame("");
    main_frame_->set_parent(this);
    main_frame_->set_space_between(2);
    main_frame_->set_background_colour(smlt::Colour::NONE);
    main_frame_->set_border_colour(smlt::Colour::NONE);
    main_frame_->set_foreground_colour(smlt::Colour::NONE);

    panel_ = KeyboardPanel::create(config, stage);
    main_frame_->pack_child(panel_.get());
}

Keyboard::~Keyboard() {
    main_frame_->unpack_child(panel_.get(), CHILD_CLEANUP_RETAIN);
    panel_.reset();
}

void Keyboard::activate() {

}

void Keyboard::set_mode(KeyboardMode mode) {
    if(mode_ == mode) {
        return;
    }

    mode_ = mode;
}

void Keyboard::set_keyboard_integration_enabled(bool value) {
    if(value && !keyboard_listener_) {
        keyboard_listener_ = std::make_shared<KeyboardListener>(this);
    } else if(!value) {
        keyboard_listener_.reset();
    }
}

void Keyboard::set_font(FontPtr font) {
    panel_->set_font(font);
    panel_->rebuild();
}

void Keyboard::cursor_up() {
    auto focused = panel_->focused_key_;
    if(focused) {
        auto new_key = panel_->find_key(focused->x, focused->y - 1);
        if(new_key) {
            panel_->focus_key(new_key->x, new_key->y);
        }
    }
}

void Keyboard::cursor_down() {
    auto focused = panel_->focused_key_;
    if(focused) {
        auto new_key = panel_->find_key(focused->x, focused->y + 1);
        if(new_key) {
            panel_->focus_key(new_key->x, new_key->y);
        }
    }
}

void Keyboard::cursor_right() {
    auto focused = panel_->focused_key_;
    if(focused) {
        auto new_key = panel_->find_key(focused->x + 1, focused->y);
        if(new_key) {
            panel_->focus_key(new_key->x, new_key->y);
        }
    }
}

void Keyboard::cursor_left() {
    auto focused = panel_->focused_key_;
    if(focused) {
        auto new_key = panel_->find_key(focused->x - 1, focused->y);
        if(new_key) {
            panel_->focus_key(new_key->x, new_key->y);
        }
    }
}

}
}
