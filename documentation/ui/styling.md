# Styling and Themes

The UI system provides flexible styling options through individual widget properties, shared styles, and theme configurations.

## The Widget Style Model

Every widget has a `WidgetStyle` that controls its appearance. Styles are composed of four visual layers:

1. **Border** - Outer edge
2. **Background** - Main fill area
3. **Foreground** - Context-sensitive overlay (progress fill, icons, etc.)
4. **Text** - Label content

```
┌─────────────────────────┐
│        Border           │
│  ┌───────────────────┐  │
│  │     Background    │  │
│  │  ┌─────────────┐  │  │
│  │  │ Foreground  │  │  │
│  │  │  ┌───────┐  │  │  │
│  │  │  │ Text  │  │  │  │
│  │  │  └───────┘  │  │  │
│  │  └─────────────┘  │  │
│  └───────────────────┘  │
└─────────────────────────┘
```

## Basic Styling

Set properties directly on widgets:

### Colors

```cpp
widget->set_background_color(Colour(0.2f, 0.2f, 0.2f));
widget->set_foreground_color(Colour(0.5f, 0.5f, 0.5f));
widget->set_border_color(Colour::WHITE);
widget->set_text_color(Colour(255, 255, 255, 255));
```

### Border

```cpp
widget->set_border_width(2);
widget->set_border_radius(4);  // Rounded corners
widget->set_border_color(Colour::WHITE);
```

### Padding

```cpp
// Uniform padding on all sides
widget->set_padding(10);

// Per-side padding (left, right, bottom, top)
widget->set_padding(5, 5, 10, 10);
```

### Opacity

```cpp
widget->set_opacity(0.5f);  // 50% transparent
```

## UIConfig Themes

`UIConfig` defines a complete theme with defaults for all widget types. Apply themes globally or per-widget.

### Default Theme Values

The built-in `UIConfig` uses these defaults:

```cpp
UIConfig config;

// Colors
config.foreground_color_ = Color::from_bytes(40, 40, 40, 255);
config.background_color_ = Color::from_bytes(53, 53, 53, 255);
config.text_color_ = Color::from_bytes(219, 219, 219, 255);
config.highlight_color_ = Color::from_bytes(0, 51, 102, 255);

// Typography
config.font_family_ = "";  // Use engine default
config.font_size_ = Px(0); // Use engine default
config.line_height_ = Rem(1.5f);

// Label defaults
config.label_padding_ = { Px(4), Px(4), Px(4), Px(4) };
config.label_background_color_ = Color::none();
config.label_foreground_color_ = Color::none();
config.label_border_color_ = Color::none();
config.label_text_color_ = config.text_color_;
config.label_resize_mode_ = RESIZE_MODE_FIT_CONTENT;

// Button defaults
config.button_padding_ = { Px(30), Px(30), Px(20), Px(20) };
config.button_background_color_ = config.highlight_color_;
config.button_foreground_color_ = Color::none();
config.button_text_color_ = config.text_color_;
config.button_border_color_ = Color::none();
config.button_border_width_ = Px(0);
config.button_border_radius_ = Px(4);
config.button_resize_mode_ = RESIZE_MODE_FIT_CONTENT;

// Progress bar defaults
config.progress_bar_foreground_color_ = config.highlight_color_;
config.progress_bar_background_color_ = config.background_color_;
config.progress_bar_border_color_ = config.foreground_color_;
config.progress_bar_text_color_ = config.text_color_;
config.progress_bar_border_width_ = Px(2);

// Frame defaults
config.frame_background_color_ = config.background_color_;
config.frame_titlebar_color_ = config.foreground_color_;
config.frame_text_color_ = config.text_color_;
config.frame_border_width_ = Px(2);
config.frame_border_color_ = config.foreground_color_;
```

### Applying a Global Theme

Set the theme on the UIManager:

```cpp
UIConfig dark_theme;
dark_theme.background_color_ = Colour(0.1f, 0.1f, 0.1f);
dark_theme.text_color_ = Colour::WHITE;
dark_theme.highlight_color_ = Colour(0.2f, 0.6f, 0.9f);

dark_theme.button_background_color_ = Colour(0.2f, 0.2f, 0.2f);
dark_theme.button_text_color_ = Colour::WHITE;
dark_theme.button_border_color_ = Colour(0.4f, 0.4f, 0.4f);
dark_theme.button_border_radius_ = Px(6);

dark_theme.frame_background_color_ = Colour(0.15f, 0.15f, 0.15f);
dark_theme.frame_titlebar_color_ = Colour(0.25f, 0.25f, 0.25f);

ui->set_config(dark_theme);
```

### Per-Widget Themes

Override the theme on individual widgets:

```cpp
UIConfig red_theme;
red_theme.background_color_ = Colour(0.8f, 0.1f, 0.1f);
red_theme.text_color_ = Colour::WHITE;

auto danger_button = ui->new_widget_as_button("Delete", x, y);
danger_button->set_theme(red_theme);
```

## Shared Styles

Multiple widgets can share the same `WidgetStyle` object, ensuring consistent appearance:

```cpp
// Create a shared style
auto button_style = std::make_shared<WidgetStyle>();
button_style->background_color_ = Colour(0.3f, 0.3f, 0.3f);
button_style->text_color_ = Colour::WHITE;
button_style->border_color_ = Colour(0.5f, 0.5f, 0.5f);
button_style->border_width_ = Px(1);
button_style->border_radius_ = Px(4);

// Apply to widgets via params
auto params = Params()
    .set("shared_style", std::weak_ptr<WidgetStyle>(button_style));

auto btn1 = ui->create_widget<Button>("Play", params, x, y);
auto btn2 = ui->create_widget<Button>("Settings", params, x, y);
```

> **Note**: When widgets share a style, changes to the style affect all widgets using it. This is useful for creating widget groups that update together (e.g., hover states).

## Widget-Specific Styling

### Labels

Labels have no background or border by default:

```cpp
auto label = ui->new_widget_as_label("Title", x, y);

// Typography
label->set_font_size(32);
label->set_text_color(Colour::WHITE);
label->set_text_alignment(TEXT_ALIGN_CENTER);

// Optional background
label->set_background_color(Colour(0.1f, 0.1f, 0.1f));
label->set_padding(10);
```

### Buttons

Buttons are styled with a background by default:

```cpp
auto button = ui->new_widget_as_button("Click Me", x, y);

// Colors
button->set_background_color(Colour(0.2f, 0.5f, 0.8f));
button->set_text_color(Colour::WHITE);

// Border
button->set_border_width(2);
button->set_border_color(Colour::WHITE);
button->set_border_radius(8);

// Padding
button->set_padding(40, 40, 25, 25);  // Generous click area
```

### Images

Image widgets display textures with optional backgrounds:

```cpp
auto texture = assets->load_texture("ui/icon.png");
auto image = ui->new_widget_as_image(texture, x, y);

// Set size
image->resize(64, 64);

// Background (appears behind the image)
image->set_background_color(Colour(0.1f, 0.1f, 0.1f));

// Source rectangle (for sprite sheets)
image->set_source_rect(Vec2(0, 0), Vec2(32, 32));
```

### Progress Bars

Progress bars use the foreground layer as the fill indicator:

```cpp
auto progress = ui->new_widget_as_progress_bar(x, y, 200, 20);

// Bar styling
progress->set_background_color(Colour(0.2f, 0.2f, 0.2f));
progress->set_foreground_color(Colour::GREEN);
progress->set_border_width(2);
progress->set_border_color(Colour(0.4f, 0.4f, 0.4f));

// Value
progress->set_value(0.75f);  // 75%
```

### Frames

Frames use the foreground layer as a title bar background:

```cpp
auto frame = ui->new_widget_as_frame("Settings", x, y, 300, 400);

// Frame body
frame->set_background_color(Colour(0.15f, 0.15f, 0.15f));
frame->set_border_width(2);
frame->set_border_color(Colour(0.3f, 0.3f, 0.3f));

// Title bar (foreground)
frame->set_foreground_color(Colour(0.25f, 0.25f, 0.25f));
frame->set_text_color(Colour::WHITE);
```

## Background Images

Set a texture as the widget background:

```cpp
auto texture = assets->load_texture("ui/panel-bg.png");
widget->set_background_image(texture);

// Use a region of the texture (texel coordinates)
widget->set_background_image_source_rect(
    UICoord(0, 0),    // Bottom-left
    UICoord(256, 64)  // Size
);
```

Clear the background image:

```cpp
widget->set_background_image(nullptr);
```

## Foreground Images

Set a texture as the widget foreground:

```cpp
auto icon = assets->load_texture("ui/check-icon.png");
checkbox->set_foreground_image(icon);
checkbox->set_foreground_image_source_rect(
    UICoord(0, 0),
    UICoord(16, 16)
);
```

## Fonts

Configure fonts on individual widgets:

```cpp
// Using pixel size
widget->set_font("Roboto", Px(16), FONT_WEIGHT_NORMAL, FONT_STYLE_NORMAL);

// Using relative size (multiplied by theme font size)
widget->set_font("Roboto", Rem(1.5f), FONT_WEIGHT_BOLD);

// Direct font object
FontPtr font = assets->load_font("Roboto", 16);
widget->set_font(font);
```

### Font Weights and Styles

```cpp
FONT_WEIGHT_NORMAL
FONT_WEIGHT_BOLD

FONT_STYLE_NORMAL
FONT_STYLE_ITALIC
```

## Color Values

Colors can be specified in several ways:

```cpp
// Named colors
Colour::WHITE
Colour::BLACK
Colour::RED
Colour::GREEN
Colour::BLUE

// RGB float (0.0 - 1.0)
Colour(0.5f, 0.5f, 0.5f)

// RGBA float
Colour(0.5f, 0.5f, 0.5f, 1.0f)

// From bytes (0 - 255)
Colour::from_bytes(128, 128, 128, 255)

// Transparent (disables a layer)
Colour::none()
```

### Disabling Layers

Set a layer color to `Colour::none()` to disable rendering of that layer for performance:

```cpp
// Remove border entirely
widget->set_border_color(Colour::none());

// Remove background (transparent)
widget->set_background_color(Colour::none());

// Hide foreground
widget->set_foreground_color(Colour::none());
```

## Building Themes

Create reusable theme configurations:

### Dark Theme

```cpp
UIConfig dark_theme;
dark_theme.background_color_ = Colour::from_bytes(30, 30, 30);
dark_theme.foreground_color_ = Colour::from_bytes(50, 50, 50);
dark_theme.text_color_ = Colour::from_bytes(220, 220, 220);
dark_theme.highlight_color_ = Colour::from_bytes(0, 120, 215);

dark_theme.button_background_color_ = Colour::from_bytes(55, 55, 55);
dark_theme.button_text_color_ = Colour::from_bytes(220, 220, 220);
dark_theme.button_border_color_ = Colour::from_bytes(80, 80, 80);
dark_theme.button_border_radius_ = Px(4);

dark_theme.frame_background_color_ = Colour::from_bytes(40, 40, 40);
dark_theme.frame_titlebar_color_ = Colour::from_bytes(55, 55, 55);
dark_theme.frame_text_color_ = Colour::from_bytes(220, 220, 220);
dark_theme.frame_border_color_ = Colour::from_bytes(70, 70, 70);
```

### Light Theme

```cpp
UIConfig light_theme;
light_theme.background_color_ = Colour::from_bytes(245, 245, 245);
light_theme.foreground_color_ = Colour::from_bytes(200, 200, 200);
light_theme.text_color_ = Colour::from_bytes(30, 30, 30);
light_theme.highlight_color_ = Colour::from_bytes(0, 120, 215);

light_theme.button_background_color_ = Colour::from_bytes(220, 220, 220);
light_theme.button_text_color_ = Colour::from_bytes(30, 30, 30);
light_theme.button_border_color_ = Colour::from_bytes(180, 180, 180);
light_theme.button_border_radius_ = Px(4);

light_theme.frame_background_color_ = Colour::from_bytes(255, 255, 255);
light_theme.frame_titlebar_color_ = Colour::from_bytes(230, 230, 230);
light_theme.frame_text_color_ = Colour::from_bytes(30, 30, 30);
light_theme.frame_border_color_ = Colour::from_bytes(200, 200, 200);
```

### Applying Themes at Runtime

Switch themes dynamically:

```cpp
void apply_theme(const std::string& theme_name) {
    if (theme_name == "dark") {
        ui->set_config(dark_theme);
    } else if (theme_name == "light") {
        ui->set_config(light_theme);
    }
}
```

## Complete Styling Example

A themed inventory panel:

```cpp
class InventoryUI : public StageNode {
private:
    UIConfig panel_theme_;

    void setup_theme() {
        panel_theme_.background_color_ = Colour::from_bytes(40, 35, 30);
        panel_theme_.foreground_color_ = Colour::from_bytes(60, 55, 50);
        panel_theme_.text_color_ = Colour::from_bytes(210, 190, 170);
        panel_theme_.highlight_color_ = Colour::from_bytes(180, 140, 60);

        panel_theme_.frame_background_color_ = panel_theme_.background_color_;
        panel_theme_.frame_titlebar_color_ = panel_theme_.foreground_color_;
        panel_theme_.frame_text_color_ = panel_theme_.text_color_;
        panel_theme_.frame_border_width_ = Px(2);
        panel_theme_.frame_border_color_ = Colour::from_bytes(100, 80, 50);

        panel_theme_.button_background_color_ = Colour::from_bytes(50, 45, 40);
        panel_theme_.button_text_color_ = panel_theme_.text_color_;
        panel_theme_.button_border_color_ = Colour::from_bytes(100, 80, 50);
        panel_theme_.button_border_radius_ = Px(3);
    }

    void on_load() override {
        setup_theme();
        ui->set_config(panel_theme_);

        // Inventory panel with title bar
        auto panel = ui->new_widget_as_frame("Inventory", 50, 50, 300, 400);
        panel->set_space_between(5);
        panel->set_padding(10);

        // Item slots (grid using nested frames)
        auto slots_frame = ui->new_widget_as_frame("", 0, 0, 0, 0);
        slots_frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
        slots_frame->set_space_between(5);

        for (int row = 0; row < 4; ++row) {
            auto slot_row = ui->new_widget_as_frame("", 0, 0, 0, 0);
            slot_row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
            slot_row->set_space_between(5);

            for (int col = 0; col < 5; ++col) {
                auto slot = ui->new_widget_as_button("", 0, 0);
                slot->resize(50, 50);
                slot->set_background_color(Colour::from_bytes(60, 55, 50));
                slot_row->pack_child(slot);
            }

            slots_frame->pack_child(slot_row);
        }

        panel->pack_child(slots_frame);

        // Action buttons
        auto actions_frame = ui->new_widget_as_frame("", 0, 0, 0, 0);
        actions_frame->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
        actions_frame->set_space_between(10);

        auto equip_btn = ui->new_widget_as_button("Equip", 0, 0);
        equip_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        equip_btn->set_width(80);

        auto drop_btn = ui->new_widget_as_button("Drop", 0, 0);
        drop_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        drop_btn->set_width(80);
        drop_btn->set_background_color(Colour::from_bytes(120, 40, 40));

        actions_frame->pack_child(equip_btn);
        actions_frame->pack_child(drop_btn);
        panel->pack_child(actions_frame);
    }
};
```

## Performance Tips

### 1. Disable Unused Layers

Setting a layer color to `Colour::none()` prevents it from being rendered:

```cpp
// Good: Label with no unnecessary layers
auto label = ui->new_widget_as_label("Score", x, y);
label->set_background_color(Colour::none());
label->set_border_color(Colour::none());
label->set_foreground_color(Colour::none());
```

### 2. Minimize Style Changes

Avoid changing styles every frame:

```cpp
// Bad: Updating style every frame
void on_update(float dt) override {
    health_bar->set_foreground_color(health_color());
}

// Good: Only update when the value changes
void on_health_changed(float health) {
    health_bar->set_foreground_color(health_color(health));
}
```

### 3. Share Styles for Widget Groups

When multiple widgets need the same style, use a shared style to reduce memory and ensure consistency.

### 4. Use Themes Over Per-Widget Styling

For large UI systems, define themes via `UIConfig` rather than styling each widget individually. This is more maintainable and performant.

## Troubleshooting

### Widget Not Showing Background

- Ensure the background color is not set to `Colour::none()`
- Check that the widget has enough size for the background to render

### Border Not Visible

- Verify `border_width` is greater than 0
- Ensure `border_color` is not `Colour::none()`

### Text Not Appearing

- Check `text_color` is not `Colour::none()`
- Verify the font is loaded and sized appropriately
- Ensure the widget is large enough to display the text

### Rounded Corners Not Working

- Set `border_radius` to a value greater than 0
- Border radius is capped at half the smallest widget dimension

## See Also

- **[UI Overview](overview.md)** - UI system introduction
- **[Layouts](layouts.md)** - Frame layout system
- **[Widgets Reference](widgets.md)** - Widget API details
