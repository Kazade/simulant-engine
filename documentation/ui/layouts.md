# Layouts and Frames

Frames are container widgets that organize child widgets in rows or columns. They provide automatic layout management, eliminating the need to manually position every widget.

## Frame Basics

A Frame is a container that packs child widgets sequentially in a specified direction. Frames can optionally display a title (using the widget's text and foreground layers as a title bar).

```cpp
// Create a frame
auto frame = ui->new_widget_as_frame("Main Menu", x, y, 300, 400);
```

### Layout Directions

Frames support two layout directions:

```cpp
// Vertical layout (default) - children stack top to bottom
frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);

// Horizontal layout - children arrange left to right
frame->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
```

### Spacing

Control the gap between child widgets:

```cpp
frame->set_space_between(10);  // 10 pixels between children
```

## Packing Widgets

Add widgets to a frame using `pack_child()`:

```cpp
auto frame = ui->new_widget_as_frame("", x, y, 300, 200);
frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
frame->set_space_between(5);

// Create and pack buttons
auto play_btn = ui->new_widget_as_button("Play", 0, 0);
frame->pack_child(play_btn);

auto settings_btn = ui->new_widget_as_button("Settings", 0, 0);
frame->pack_child(settings_btn);

auto quit_btn = ui->new_widget_as_button("Quit", 0, 0);
frame->pack_child(quit_btn);
```

### Using create_child

You can also create widgets as children of a frame directly:

```cpp
auto frame = ui->new_widget_as_frame("", x, y, 300, 200);
frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);

auto button = frame->create_child<Button>("Play", 0, 0);
frame->pack_child(button);
```

### Unpacking Widgets

Remove a widget from a frame:

```cpp
// Remove and destroy the widget
frame->unpack_child(widget, CHILD_CLEANUP_DESTROY);

// Remove but keep the widget alive
frame->unpack_child(widget, CHILD_CLEANUP_RETAIN);
```

### Accessing Packed Children

Iterate over a frame's children:

```cpp
for (auto* child : frame->packed_children()) {
    S_INFO("Child widget at frame");
    // Modify children as needed
    child->set_visible(false);
}
```

## How Layout Works

Frames calculate their content dimensions based on their children:

### Vertical Layout (TOP_TO_BOTTOM)

- Content width = widest child
- Content height = sum of all child heights + spacing

```
┌─────────────────────────┐
│      Frame Title        │
├─────────────────────────┤
│  ┌───────────────────┐  │
│  │   Child 1         │  │
│  └───────────────────┘  │
│         (spacing)       │
│  ┌───────────────────┐  │
│  │   Child 2         │  │
│  └───────────────────┘  │
│         (spacing)       │
│  ┌───────────────────┐  │
│  │   Child 3         │  │
│  └───────────────────┘  │
└─────────────────────────┘
```

### Horizontal Layout (LEFT_TO_RIGHT)

- Content width = sum of all child widths + spacing
- Content height = tallest child

```
┌─────────────────────────────────────────────┐
│  ┌──────┐  ┌──────────┐  ┌────────────┐    │
│  │Child1│  │  Child2  │  │   Child3   │    │
│  └──────┘  └──────────┘  └────────────┘    │
└─────────────────────────────────────────────┘
```

## Title Bars

When a frame has text set, the text and foreground layers render as a title bar:

```cpp
auto frame = ui->new_widget_as_frame("Options", x, y, 300, 400);
// "Options" appears as the title bar
// Foreground color becomes the title bar background
frame->set_foreground_color(Colour(0.3f, 0.3f, 0.3f));
frame->set_text_color(Colour::WHITE);
```

The title bar height equals the line height of the frame's font.

## Sizing Frames

Frames follow the same sizing rules as other widgets:

### Fixed Size

```cpp
frame->set_resize_mode(RESIZE_MODE_FIXED);
frame->resize(300, 400);
// Frame stays at 300x400 regardless of content
```

### Fit Content

```cpp
frame->set_resize_mode(RESIZE_MODE_FIT_CONTENT);
frame->resize(0, 0);
// Frame expands/shrinks to fit children
```

### Fixed Width, Dynamic Height

```cpp
frame->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
frame->resize(300, 0);
// Width stays at 300, height adjusts to content
```

### Fixed Height, Dynamic Width

```cpp
frame->set_resize_mode(RESIZE_MODE_FIXED_HEIGHT);
frame->resize(0, 400);
// Height stays at 400, width adjusts to content
```

## Nested Frames

Frames can contain other frames, enabling complex layouts:

```cpp
// Outer vertical frame
auto outer_frame = ui->new_widget_as_frame("", x, y, 400, 500);
outer_frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
outer_frame->set_space_between(10);

// Inner horizontal frame (for a row of buttons)
auto button_row = ui->new_widget_as_frame("", 0, 0, 0, 0);
button_row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
button_row->set_space_between(5);

auto ok_btn = ui->new_widget_as_button("OK", 0, 0);
auto cancel_btn = ui->new_widget_as_button("Cancel", 0, 0);

button_row->pack_child(ok_btn);
button_row->pack_child(cancel_btn);

// Pack the inner frame into the outer frame
outer_frame->pack_child(button_row);
```

## Anchor Points

Widgets (including frames) have an anchor point that determines their origin for positioning:

```cpp
// Anchor at center (default for most widgets)
widget->set_anchor_point(0.5f, 0.5f);

// Anchor at top-left
widget->set_anchor_point(0.0f, 1.0f);

// Anchor at bottom-left
widget->set_anchor_point(0.0f, 0.0f);
```

The anchor point uses a normalized coordinate system where `(0, 0)` is bottom-left and `(1, 1)` is top-right.

## Complete Layout Example

Building a settings panel with nested frames:

```cpp
class SettingsScene : public Scene<SettingsScene> {
private:
    CameraID ui_camera_;

    void on_load() override {
        ui_camera_ = create_child<Camera2D>()->id();

        // Main container
        auto panel = ui->new_widget_as_frame("Settings", 300, 100, 400, 500);
        panel->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
        panel->set_space_between(15);
        panel->set_padding(10);

        // Volume section
        auto volume_frame = ui->new_widget_as_frame("Volume", 0, 0, 0, 0);
        volume_frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
        volume_frame->set_space_between(5);

        auto volume_label = ui->new_widget_as_label("Master Volume", 0, 0);
        auto volume_bar = ui->new_widget_as_progress_bar(0, 0, 300, 20);
        volume_bar->set_value(0.8f);

        volume_frame->pack_child(volume_label);
        volume_frame->pack_child(volume_bar);

        // Audio device section
        auto device_frame = ui->new_widget_as_frame("Audio Device", 0, 0, 0, 0);
        device_frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
        device_frame->set_space_between(5);

        auto device_label = ui->new_widget_as_label("Output Device", 0, 0);
        auto device_dropdown = ui->new_widget_as_button("Default Device", 0, 0);

        device_frame->pack_child(device_label);
        device_frame->pack_child(device_dropdown);

        // Buttons row
        auto buttons_frame = ui->new_widget_as_frame("", 0, 0, 0, 0);
        buttons_frame->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
        buttons_frame->set_space_between(10);

        auto apply_btn = ui->new_widget_as_button("Apply", 0, 0);
        apply_btn->signal_activated().connect([this]() {
            S_INFO("Settings applied");
        });

        auto close_btn = ui->new_widget_as_button("Close", 0, 0);
        close_btn->signal_activated().connect([this]() {
            scenes->load_and_activate("menu");
        });

        buttons_frame->pack_child(apply_btn);
        buttons_frame->pack_child(close_btn);

        // Pack everything into the main panel
        panel->pack_child(volume_frame);
        panel->pack_child(device_frame);
        panel->pack_child(buttons_frame);

        // Render pipeline
        auto layer = compositor->create_layer(this, camera(ui_camera_));
        layer->set_priority(RENDER_PRIORITY_FOREGROUND);
        link_pipeline(layer);
    }
};
```

## Styling Frames

Frames have dedicated theme properties in `UIConfig`:

```cpp
UIConfig config;
config.frame_background_color_ = Colour(0.15f, 0.15f, 0.15f);
config.frame_titlebar_color_ = Colour(0.25f, 0.25f, 0.25f);
config.frame_text_color_ = Colour::WHITE;
config.frame_border_width_ = Px(2);
config.frame_border_color_ = Colour(0.4f, 0.4f, 0.4f);

ui->set_config(config);
```

Or set styles per-widget:

```cpp
frame->set_background_color(Colour(0.1f, 0.1f, 0.1f));
frame->set_foreground_color(Colour(0.3f, 0.3f, 0.3f));
frame->set_border_color(Colour::WHITE);
frame->set_border_width(2);
frame->set_text_color(Colour::WHITE);
```

## Best Practices

### 1. Prefer Frames Over Manual Positioning

```cpp
// Good: Layout manages positions automatically
auto frame = ui->new_widget_as_frame("", x, y, w, h);
frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
frame->pack_child(button1);
frame->pack_child(button2);

// Bad: Manual positioning is fragile and hard to maintain
// button1->move_to(100, 100);
// button2->move_to(100, 150);
```

### 2. Use Fit Content for Dynamic Layouts

Let frames size themselves to their content:

```cpp
frame->set_resize_mode(RESIZE_MODE_FIT_CONTENT);
frame->resize(0, 0);
```

### 3. Group Related Widgets in Nested Frames

Organize complex UI into logical sections:

```cpp
auto main_frame = ui->new_widget_as_frame("Main", x, y, w, h);
auto section_a = ui->new_widget_as_frame("Section A", 0, 0, 0, 0);
auto section_b = ui->new_widget_as_frame("Section B", 0, 0, 0, 0);

// Pack widgets into sections
// Pack sections into main frame
main_frame->pack_child(section_a);
main_frame->pack_child(section_b);
```

### 4. Set Space Between for Consistent Gaps

```cpp
frame->set_space_between(10);  // Consistent spacing
// Rather than adding margins to individual children
```

## Troubleshooting

### Children Not Appearing

- Ensure children are packed into the frame with `pack_child()`
- Check the frame has enough size to contain its children
- Verify the frame and children are visible

### Layout Not Updating

When you modify children or their sizes, the frame rebuilds automatically. If this doesn't happen:

```cpp
frame->rebuild();  // Force a layout recalculation
```

### Title Bar Not Showing

- The frame must have text set (passed to `new_widget_as_frame()` or via `set_text()`)
- The foreground color must be set (it renders as the title bar background)

## See Also

- **[UI Overview](overview.md)** - UI system introduction
- **[Widgets Reference](widgets.md)** - Widget API details
- **[Styling](styling.md)** - Themes and customization
