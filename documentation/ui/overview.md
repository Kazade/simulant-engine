# UI System Overview

Simulant includes a comprehensive UI system for creating in-game interfaces, menus, HUDs, and interactive elements. The UI system uses a CSS-like box model and provides a range of built-in widgets.

## Architecture

The UI system is built on top of the StageNode hierarchy:

```
Scene
└── UIManager (manages UI input routing)
    └── Widget (base class for all UI elements)
        ├── Label
        ├── Button
        ├── Image
        ├── ProgressBar
        ├── TextEntry
        ├── Frame (layout container)
        └── Keyboard (on-screen keyboard)
```

## Quick Start

### Creating a UI

1. Create a UIManager
2. Create widgets through the UI manager
3. Position and configure widgets
4. Render with a 2D camera

```cpp
class MenuScene : public Scene<MenuScene> {
private:
    CameraID ui_camera_;
    UIManager* ui_manager_;
    
    void on_load() override {
        // Create 2D camera for UI
        ui_camera_ = create_child<Camera2D>()->id();
        
        // Get UI manager
        ui_manager_ = ui;  // 'ui' is a property on Scene
        
        // Create UI elements
        create_menu();
        
        // Create render pipeline
        auto layer = compositor->create_layer(this, camera(ui_camera_));
        layer->set_priority(RENDER_PRIORITY_FOREGROUND);
        link_pipeline(layer);
    }
    
    void create_menu() {
        auto title = ui_manager_->new_widget_as_label("My Game", 100, 50);
        title->set_font_size(32);
        title->set_text_align(TEXT_ALIGN_CENTER);
        
        auto play_button = ui_manager_->new_widget_as_button("Play", 100, 100);
        play_button->on_click.connect([this]() {
            scenes->load_and_activate("game");
        });
    }
};
```

## Widgets

### Label

Displays text:

```cpp
auto label = ui->new_widget_as_label("Hello World", x, y);
label->set_text("New Text");
label->set_font_size(24);
label->set_text_color(Colour::WHITE);
label->set_text_align(TEXT_ALIGN_CENTER);
```

### Button

Clickable button with optional text:

```cpp
auto button = ui->new_widget_as_button("Click Me", x, y);
button->set_text("New Label");

// Handle clicks
button->signal_activated().connect([]() {
    S_INFO("Button clicked!");
});

// Visual customization
button->set_background_color(Colour(0.2f, 0.5f, 0.8f));
button->set_border_width(2);
button->set_border_color(Colour::WHITE);
```

### Image

Display a texture:

```cpp
auto texture = assets->load_texture("ui/icon.png");
auto image = ui->new_widget_as_image(texture, x, y);
image->set_size(64, 64);

// Change source rectangle (for sprite sheets)
image->set_source_rect(Vec2(0, 0), Vec2(32, 32));
```

### ProgressBar

Show progress/values:

```cpp
auto progress = ui->new_widget_as_progress_bar(x, y, 200, 20);
progress->set_value(0.75f);  // 75% full (0.0 to 1.0)
progress->set_fill_color(Colour::GREEN);
progress->set_background_color(Colour::GRAY);
```

### TextEntry

Text input field:

```cpp
auto text_entry = ui->new_widget_as_text_entry(x, y, 200, 30);
text_entry->set_text("Enter name...");
text_entry->set_max_length(20);

text_entry->signal_done().connect([this]() {
    S_INFO("User entered: {}", text_entry->text());
});
```

## Widget Sizing

Widgets support multiple resize modes:

### Resize Modes

```cpp
// Fixed size (doesn't change)
widget->set_resize_mode(RESIZE_MODE_FIXED);
widget->set_size(200, 50);

// Fixed width, height adjusts to content
widget->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
widget->set_size(200, 0);  // Height will adjust

// Fixed height, width adjusts
widget->set_resize_mode(RESIZE_MODE_FIXED_HEIGHT);
widget->set_size(0, 50);

// Both dimensions adjust to content
widget->set_resize_mode(RESIZE_MODE_FIT_CONTENT);
widget->set_size(0, 0);
```

### Box Model

Widgets use a CSS-like box model:

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

Configure the box model:

```cpp
widget->set_border_width(2);
widget->set_padding(10);              // Internal spacing
widget->set_background_color(Colour::BLUE);
widget->set_border_color(Colour::WHITE);
```

## Layout with Frames

Frames organize widgets in rows or columns:

### Creating a Frame

```cpp
auto frame = ui->new_widget_as_frame("Menu", x, y, 300, 400);
frame->set_layout_direction(LAYOUT_DIRECTION_VERTICAL);
frame->set_padding(10);
frame->set_spacing(5);  // Space between children
```

### Adding Widgets to Frames

```cpp
// Widgets become children of the frame
auto button1 = frame->create_child<Button>("Button 1", 0, 0);
button1->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
button1->set_width(280);

auto button2 = frame->create_child<Button>("Button 2", 0, 0);
button2->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
button2->set_width(280);
```

### Frame Options

```cpp
frame->set_layout_direction(LAYOUT_DIRECTION_HORIZONTAL);
frame->set_layout_direction(LAYOUT_DIRECTION_VERTICAL);  // Default

frame->set_spacing(10);        // Gap between children
frame->set_padding(5);         // Internal padding
frame->set_pack_start(true);   // Pack from start (top/left)
```

## Input Handling

UI widgets handle input automatically through the UIManager:

### Mouse Input

```cpp
button->signal_pointer_down().connect([]() {
    S_INFO("Mouse down on button");
});

button->signal_pointer_up().connect([]() {
    S_INFO("Mouse up on button");
});

button->signal_activated().connect([]() {
    S_INFO("Button clicked");
});
```

### Multi-Touch

The UI system supports multi-touch:

```cpp
// Each touch has a unique ID
widget->signal_pointer_down().connect([](int touch_id, Vec2 pos) {
    S_INFO("Touch {} at {}", touch_id, pos);
});
```

### Keyboard Navigation

Widgets support focus chain navigation:

```cpp
widget->set_focusable(true);
widget->focus();

// Navigate focus
ui->focus_next();
ui->focus_previous();

// Get focused widget
Widget* focused = ui->focused_widget();
```

## Styling

### UIConfig

Create reusable themes:

```cpp
UIConfig config;
config.normal_color = Colour(0.2f, 0.2f, 0.2f);
config.hover_color = Colour(0.3f, 0.3f, 0.3f);
config.active_color = Colour(0.4f, 0.4f, 0.4f);
config.font_size = 16;

ui_manager_->set_config(config);
```

### Widget Layers

Widgets have multiple visual layers:

```cpp
// Border layer
widget->set_border_width(2);
widget->set_border_color(Colour::WHITE);

// Background layer
widget->set_background_color(Colour::BLUE);

// Foreground layer (varies by widget type)
widget->set_foreground_visible(true);
widget->set_foreground_color(Colour::GREEN);

// Text layer
widget->set_text("Hello");
widget->set_text_color(Colour::WHITE);
widget->set_font_size(24);
```

### Progress Bar Foreground

For progress bars, the foreground is the fill indicator:

```cpp
progress->set_foreground_visible(true);
progress->set_foreground_color(Colour::GREEN);
// Fill amount controlled by set_value()
```

### Button Foreground

For buttons, foreground can be an icon:

```cpp
button->set_foreground_visible(true);
button->set_foreground_texture(icon_texture);
```

## Text Entry & Keyboard

### Text Entry Widget

```cpp
auto text_entry = ui->new_widget_as_text_entry(x, y, 200, 30);
text_entry->set_text("Default");
text_entry->set_placeholder("Enter text...");

text_entry->signal_text_changed().connect([](const std::string& text) {
    S_INFO("Text changed to: {}", text);
});

text_entry->signal_done().connect([]() {
    S_INFO("Finished editing");
});
```

### On-Screen Keyboard

For touch devices or games without physical keyboards:

```cpp
auto keyboard = ui->new_widget_as_keyboard(x, y);
keyboard->set_target(text_entry);  // Auto-update text entry

// Or handle manually
keyboard->signal_activated().connect([](char c) {
    S_INFO("Key pressed: {}", c);
});

keyboard->signal_done().connect([]() {
    S_INFO("Keyboard dismissed");
});
```

> **Note**: The Keyboard widget is in alpha state. See the [Widgets Reference](widgets.md) for known limitations.

## HUD Example

Complete HUD implementation:

```cpp
class HUD : public StageNode {
private:
    Label* score_label_;
    Label* lives_label_;
    ProgressBar* health_bar_;
    
public:
    void on_load() override {
        ui_manager_ = stage_->ui;
        
        // Score
        score_label_ = ui_manager_->new_widget_as_label("Score: 0", 10, 10);
        score_label_->set_font_size(24);
        score_label_->set_text_color(Colour::WHITE);
        
        // Lives
        lives_label_ = ui_manager_->new_widget_as_label("Lives: 3", 10, 40);
        lives_label_->set_font_size(24);
        
        // Health bar
        health_bar_ = ui_manager_->new_widget_as_progress_bar(10, 70, 200, 20);
        health_bar_->set_value(1.0f);
        health_bar_->set_fill_color(Colour::GREEN);
    }
    
    void update_score(int score) {
        score_label_->set_text("Score: " + std::to_string(score));
    }
    
    void update_lives(int lives) {
        lives_label_->set_text("Lives: " + std::to_string(lives));
    }
    
    void update_health(float health) {
        health_bar_->set_value(health);
        
        // Change color based on health
        if (health > 0.6f) {
            health_bar_->set_fill_color(Colour::GREEN);
        } else if (health > 0.3f) {
            health_bar_->set_fill_color(Colour::YELLOW);
        } else {
            health_bar_->set_fill_color(Colour::RED);
        }
    }
};
```

## Menu Example

Complete main menu:

```cpp
class MenuScene : public Scene<MenuScene> {
private:
    CameraID camera_;
    
    void on_load() override {
        // 2D camera
        camera_ = create_child<Camera2D>()->id();
        
        // Title
        auto title = ui->new_widget_as_label("My Game", 400, 100);
        title->set_font_size(48);
        title->set_text_align(TEXT_ALIGN_CENTER);
        
        // Menu frame
        auto menu_frame = ui->new_widget_as_frame("", 350, 250, 300, 200);
        menu_frame->set_layout_direction(LAYOUT_DIRECTION_VERTICAL);
        menu_frame->set_spacing(10);
        menu_frame->set_padding(10);
        
        // Buttons
        auto play_btn = menu_frame->create_child<Button>("Play", 0, 0);
        play_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        play_btn->set_width(280);
        play_btn->signal_activated().connect([this]() {
            scenes->load_and_activate("game", SCENE_CHANGE_BEHAVIOUR_UNLOAD);
        });
        
        auto settings_btn = menu_frame->create_child<Button>("Settings", 0, 0);
        settings_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        settings_btn->set_width(280);
        settings_btn->signal_activated().connect([this]() {
            scenes->load_and_activate("settings");
        });
        
        auto quit_btn = menu_frame->create_child<Button>("Quit", 0, 0);
        quit_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        quit_btn->set_width(280);
        quit_btn->signal_activated().connect([this]() {
            app->shutdown();
        });
        
        // Render pipeline
        auto layer = compositor->create_layer(this, camera(camera_));
        layer->set_priority(RENDER_PRIORITY_FOREGROUND);
        link_pipeline(layer);
    }
};
```

## Best Practices

### 1. Use a Separate Camera for UI

```cpp
// Good: Separate 2D camera for UI
auto ui_camera = create_child<Camera2D>();
auto ui_layer = compositor->create_layer(this, ui_camera);
ui_layer->set_priority(RENDER_PRIORITY_FOREGROUND);

// Bad: Rendering UI in 3D space
// auto layer = compositor->create_layer(this, perspective_camera);
```

### 2. Use Frames for Layout

```cpp
// Good: Using frame for automatic layout
auto frame = ui->new_widget_as_frame("", x, y, w, h);
frame->set_layout_direction(LAYOUT_DIRECTION_VERTICAL);

// Bad: Manual positioning
// button1->move_to(100, 100);
// button2->move_to(100, 150);
```

### 3. Use Resize Modes Appropriately

```cpp
// Good: Let text adjust size
label->set_resize_mode(RESIZE_MODE_FIT_CONTENT);

// Good: Fixed size for buttons
button->set_resize_mode(RESIZE_MODE_FIXED);
button->set_size(200, 50);
```

### 4. Handle Widget Destruction

Widgets are destroyed with their parent StageNode:

```cpp
widget->destroy();  // Removes from UI
// or
parent->destroy();  // Removes widget and all children
```

### 5. Use Localization

Make UI translatable:

```cpp
auto label = ui->new_widget_as_label(_T("Play Game"), x, y);
auto button = ui->new_widget_as_button(_T("Settings"), x, y);
```

See [Localization](localization.md) for details.

## Performance Tips

### 1. Minimize Widget Updates

Only update when values change:

```cpp
// Bad: Update every frame
void on_update(float dt) override {
    score_label_->set_text("Score: " + std::to_string(score));
}

// Good: Update only when score changes
void on_score_changed(int new_score) {
    score_label_->set_text("Score: " + std::to_string(new_score));
}
```

### 2. Use 2D Camera for UI

3D cameras add overhead for UI rendering.

### 3. Limit Active Widgets

Too many active widgets impact performance. Hide off-screen widgets:

```cpp
if (!widget->is_on_screen()) {
    widget->set_visible(false);
}
```

## Troubleshooting

### Widgets Not Showing

- Check UI camera is set up correctly
- Verify layer priority puts UI in foreground
- Ensure widget positions are within viewport

### Input Not Working

- Check UIManager exists in scene
- Verify widgets have correct size for hit testing
- Ensure widget is visible and enabled

### Text Not Displaying

- Check font is loaded
- Verify text color is visible against background
- Ensure widget size fits text content

## Next Steps

- **[Widgets Reference](widgets.md)** - Detailed widget API
- **[Layouts](layouts.md)** - Frame layout system
- **[Styling](styling.md)** - Themes and customization
- **[Localization](localization.md)** - Multi-language UI

## See Also

- **[Stage Nodes](../core-concepts/stage-nodes.md)** - Node hierarchy
- **[Cameras](../core-concepts/cameras.md)** - 2D and 3D cameras
- **[Rendering Pipelines](../rendering/pipelines.md)** - Layer system
