# Tutorial 4: Creating Menus and HUDs

In this tutorial, you will learn how to build user interfaces in Simulant. You will create a main menu with buttons, a heads-up display (HUD) with score and health, and learn how to style and organise UI elements.

**Prerequisites:** [Tutorial 1 -- Basic Application](01_basic_application.md)

**Related documentation:** [UI Overview](../ui/overview.md), [Widgets](../ui/widgets.md), [Layouts](../ui/layouts.md), [Styling](../ui/styling.md).

---

## What You Will Build

By the end of this tutorial, you will have a working application with:

- A main menu scene with a title and clickable buttons
- A game HUD with score, lives, and a health bar
- Styled UI elements with consistent theming
- Proper input handling for mouse and keyboard

---

## Step 1: Understanding the UI System

Simulant includes a comprehensive UI system built on top of the StageNode hierarchy. The key components are:

```
Scene
  └── UIManager (manages UI input routing)
      └── Widget (base class for all UI elements)
          ├── Label     -- Displays text
          ├── Button    -- Clickable button with optional text
          ├── Image     -- Displays a texture
          ├── ProgressBar -- Shows progress/values
          ├── TextEntry -- Text input field
          └── Frame     -- Layout container (rows or columns)
```

### How UI rendering works

UI widgets are rendered using a 2D camera. You create a `Camera2D`, attach widgets to the scene, and the UIManager handles input routing automatically.

---

## Step 2: Creating a Main Menu Scene

Start with the basic application structure and create a menu scene:

```cpp
#include "simulant/simulant.h"

using namespace smlt;

class MenuScene : public Scene {
public:
    MenuScene(Window* window) : Scene(window) {}

    void on_load() override {
        // We will build our menu here
    }

private:
    CameraID ui_camera_;
};

class UIDemo : public Application {
public:
    UIDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<MenuScene>("main");
        scenes->activate("menu");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "UI Demo";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    UIDemo app(config);
    return app.run(argc, argv);
}
```

---

## Step 3: Setting Up the UI Camera

Every UI needs a 2D camera. Add this to `on_load()`:

```cpp
void on_load() override {
    // Create a 2D camera for UI rendering
    ui_camera_ = create_child<Camera2D>()->id();

    // Build the menu
    create_menu();

    // Create a render layer for the UI (foreground priority)
    auto layer = compositor->create_layer(this, camera(ui_camera_));
    layer->set_priority(RENDER_PRIORITY_FOREGROUND);
    link_pipeline(layer);
}
```

The `RENDER_PRIORITY_FOREGROUND` ensures the UI renders on top of everything else.

---

## Step 4: Creating a Title and Buttons

Now build the menu content:

```cpp
void create_menu() {
    // ---- Title ----
    auto title = ui->new_widget_as_label("My Game", 640, 150);
    title->set_font_size(48);
    title->set_text_color(Color::WHITE);
    title->set_text_alignment(TEXT_ALIGN_CENTER);

    // ---- Menu frame (vertical container) ----
    auto menu_frame = ui->new_widget_as_frame("", 640, 300, 300, 250);
    menu_frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
    menu_frame->set_space_between(15);
    menu_frame->set_padding(10);

    // ---- Play button ----
    auto play_btn = ui->new_widget_as_button("Play", 0, 0);
    play_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
    play_btn->set_width(280);
    play_btn->signal_activated().connect([this]() {
        S_INFO("Play button clicked!");
        scenes->load_and_activate("main", SCENE_CHANGE_BEHAVIOUR_UNLOAD);
    });
    menu_frame->pack_child(play_btn);

    // ---- Settings button ----
    auto settings_btn = ui->new_widget_as_button("Settings", 0, 0);
    settings_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
    settings_btn->set_width(280);
    settings_btn->signal_activated().connect([this]() {
        S_INFO("Settings button clicked!");
    });
    menu_frame->pack_child(settings_btn);

    // ---- Quit button ----
    auto quit_btn = ui->new_widget_as_button("Quit", 0, 0);
    quit_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
    quit_btn->set_width(280);
    quit_btn->signal_activated().connect([this]() {
        S_INFO("Quit button clicked!");
        app->shutdown();
    });
    menu_frame->pack_child(quit_btn);
}
```

Key points:
- Widget positions (640, 150 etc.) are in screen-space coordinates
- `pack_child()` adds widgets to a Frame, which arranges them automatically
- `signal_activated()` fires when the button is clicked

---

## Step 5: Creating a Game HUD

Now let us build a HUD that shows score, lives, and a health bar. Create a separate scene for the game:

```cpp
class GameHUD : public StageNode {
private:
    Label* score_label_ = nullptr;
    Label* lives_label_ = nullptr;
    ProgressBar* health_bar_ = nullptr;

public:
    void on_load() override {
        // ---- Score label (top-left) ----
        score_label_ = stage_->ui->new_widget_as_label("Score: 0", 20, 20);
        score_label_->set_font_size(24);
        score_label_->set_text_color(Color::WHITE);

        // ---- Lives label (below score) ----
        lives_label_ = stage_->ui->new_widget_as_label("Lives: 3", 20, 50);
        lives_label_->set_font_size(24);
        lives_label_->set_text_color(Color::WHITE);

        // ---- Health bar (top-right) ----
        health_bar_ = stage_->ui->new_widget_as_progress_bar(1000, 20, 200, 20);
        health_bar_->set_value(1.0f);  // Full health
        health_bar_->set_foreground_color(Color::GREEN);
    }

    // Update methods -- call these from your game logic
    void update_score(int score) {
        score_label_->set_text("Score: " + std::to_string(score));
    }

    void update_lives(int lives) {
        lives_label_->set_text("Lives: " + std::to_string(lives));
    }

    void update_health(float health) {
        health_bar_->set_value(health);

        // Change color based on health level
        if (health > 0.6f) {
            health_bar_->set_foreground_color(Color::GREEN);
        } else if (health > 0.3f) {
            health_bar_->set_foreground_color(Color(1.0f, 1.0f, 0.0f));  // Yellow
        } else {
            health_bar_->set_foreground_color(Color::RED);
        }
    }
};
```

Add the HUD to your game scene:

```cpp
class GameScene : public Scene {
public:
    GameScene(Window* window) : Scene(window) {}

    void on_load() override {
        // UI camera
        ui_camera_ = create_child<Camera2D>()->id();

        // Create the HUD
        hud_ = create_child<GameHUD>();

        // Render layer for UI
        auto layer = compositor->create_layer(this, camera(ui_camera_));
        layer->set_priority(RENDER_PRIORITY_FOREGROUND);
        link_pipeline(layer);

        // Simulate some game state changes
        simulate_game_updates();
    }

private:
    CameraID ui_camera_;
    GameHUD* hud_ = nullptr;

    void simulate_game_updates() {
        // In a real game, these would be called from game logic
        hud_->update_score(1500);
        hud_->update_lives(2);
        hud_->update_health(0.65f);
    }
};
```

---

## Step 6: Using Frames for Layout

Frames are the backbone of good UI layout. Instead of manually positioning every widget, you organise them into containers that handle positioning automatically.

### Vertical layout (default)

Widgets stack from top to bottom:

```cpp
auto frame = ui->new_widget_as_frame("", x, y, 300, 400);
frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
frame->set_space_between(10);  // 10 pixels between children

frame->pack_child(button1);
frame->pack_child(button2);
frame->pack_child(button3);
```

### Horizontal layout

Widgets arrange left to right:

```cpp
auto button_row = ui->new_widget_as_frame("", x, y, 0, 0);
button_row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
button_row->set_space_between(5);

button_row->pack_child(ok_button);
button_row->pack_child(cancel_button);
```

### Nested frames for complex layouts

Combine frames to build sophisticated panels:

```cpp
// Main settings panel
auto panel = ui->new_widget_as_frame("Settings", 300, 100, 400, 500);
panel->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
panel->set_space_between(15);
panel->set_padding(10);

// Volume section (nested frame)
auto volume_section = ui->new_widget_as_frame("Volume", 0, 0, 0, 0);
volume_section->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
volume_section->set_space_between(5);

auto volume_label = ui->new_widget_as_label("Master Volume", 0, 0);
auto volume_bar = ui->new_widget_as_progress_bar(0, 0, 300, 20);
volume_bar->set_value(0.8f);

volume_section->pack_child(volume_label);
volume_section->pack_child(volume_bar);

// Buttons row (horizontal nested frame)
auto buttons_row = ui->new_widget_as_frame("", 0, 0, 0, 0);
buttons_row->set_layout_direction(LAYOUT_DIRECTION_LEFT_TO_RIGHT);
buttons_row->set_space_between(10);

auto apply_btn = ui->new_widget_as_button("Apply", 0, 0);
auto close_btn = ui->new_widget_as_button("Close", 0, 0);

buttons_row->pack_child(apply_btn);
buttons_row->pack_child(close_btn);

// Pack everything into the main panel
panel->pack_child(volume_section);
panel->pack_child(buttons_row);
```

---

## Step 7: Styling Your UI

### Setting colors directly

```cpp
auto button = ui->new_widget_as_button("Click Me", x, y);
button->set_background_color(Color(0.2f, 0.5f, 0.8f));
button->set_text_color(Color::WHITE);
button->set_border_width(2);
button->set_border_color(Color::WHITE);
button->set_border_radius(8);
button->set_padding(40, 40, 25, 25);  // left, right, bottom, top
```

### Applying a theme with UIConfig

Define a complete theme and apply it globally:

```cpp
void apply_dark_theme() {
    UIConfig dark_theme;
    dark_theme.background_color_ = Color::from_bytes(30, 30, 30);
    dark_theme.text_color_ = Color::from_bytes(220, 220, 220);
    dark_theme.highlight_color_ = Color::from_bytes(0, 120, 215);

    dark_theme.button_background_color_ = Color::from_bytes(55, 55, 55);
    dark_theme.button_text_color_ = Color::from_bytes(220, 220, 220);
    dark_theme.button_border_color_ = Color::from_bytes(80, 80, 80);
    dark_theme.button_border_radius_ = Px(4);

    dark_theme.frame_background_color_ = Color::from_bytes(40, 40, 40);
    dark_theme.frame_titlebar_color_ = Color::from_bytes(55, 55, 55);

    ui->set_config(dark_theme);
}
```

Call this before creating your widgets to apply the theme to all of them.

### Disabling unused layers for performance

If a widget does not need a background or border, disable those layers:

```cpp
auto label = ui->new_widget_as_label("Score", 10, 10);
label->set_background_color(Color::none());   // No background
label->set_border_color(Color::none());        // No border
label->set_foreground_color(Color::none());    // No foreground
```

---

## Step 8: Widget Resize Modes

Widgets support multiple resize modes that control how they size themselves:

```cpp
// Fixed size -- does not change
widget->set_resize_mode(RESIZE_MODE_FIXED);
widget->set_size(200, 50);

// Fixed width, height adjusts to content
widget->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
widget->set_width(200);

// Both dimensions adjust to content
widget->set_resize_mode(RESIZE_MODE_FIT_CONTENT);
widget->set_size(0, 0);
```

For buttons in a menu, `RESIZE_MODE_FIXED_WIDTH` is ideal -- the width stays constant while the height adapts to the text.

---

## Step 9: Adding an Image Widget

Display textures in your UI (useful for logos, icons, and HUD elements):

```cpp
// Load the texture
auto texture = assets->load_texture("ui/logo.png");

// Create the image widget
auto logo = ui->new_widget_as_image(texture, 640, 80);
logo->set_size(128, 64);

// Display a specific region of the texture (for sprite sheets)
logo->set_source_rect(Vec2(0, 0), Vec2(64, 64));
```

---

## Step 10: Using a Progress Bar

Progress bars are versatile -- use them for health bars, loading screens, experience meters, and more:

```cpp
auto health_bar = ui->new_widget_as_progress_bar(1000, 20, 200, 20);
health_bar->set_value(0.75f);  // 75% full (0.0 to 1.0)
health_bar->set_foreground_color(Color::GREEN);
health_bar->set_background_color(Color(0.2f, 0.2f, 0.2f));
health_bar->set_border_width(2);
health_bar->set_border_color(Color(0.4f, 0.4f, 0.4f));
```

---

## Complete Example: Full Menu and HUD Demo

```cpp
#include "simulant/simulant.h"

using namespace smlt;

// -----------------------------------------------------------
// Game HUD
// -----------------------------------------------------------
class GameHUD : public StageNode {
private:
    Label* score_label_ = nullptr;
    Label* lives_label_ = nullptr;
    ProgressBar* health_bar_ = nullptr;

public:
    void on_load() override {
        score_label_ = stage_->ui->new_widget_as_label("Score: 0", 20, 20);
        score_label_->set_font_size(24);
        score_label_->set_text_color(Color::WHITE);

        lives_label_ = stage_->ui->new_widget_as_label("Lives: 3", 20, 50);
        lives_label_->set_font_size(24);
        lives_label_->set_text_color(Color::WHITE);

        health_bar_ = stage_->ui->new_widget_as_progress_bar(1000, 20, 200, 20);
        health_bar_->set_value(1.0f);
        health_bar_->set_foreground_color(Color::GREEN);
    }

    void update_score(int score) {
        score_label_->set_text("Score: " + std::to_string(score));
    }

    void update_lives(int lives) {
        lives_label_->set_text("Lives: " + std::to_string(lives));
    }

    void update_health(float health) {
        health_bar_->set_value(health);
        if (health > 0.6f) {
            health_bar_->set_foreground_color(Color::GREEN);
        } else if (health > 0.3f) {
            health_bar_->set_foreground_color(Color(1.0f, 1.0f, 0.0f));
        } else {
            health_bar_->set_foreground_color(Color::RED);
        }
    }
};

// -----------------------------------------------------------
// Menu Scene
// -----------------------------------------------------------
class MenuScene : public Scene {
public:
    MenuScene(Window* window) : Scene(window) {}

    void on_load() override {
        ui_camera_ = create_child<Camera2D>()->id();

        // Apply a dark theme
        UIConfig theme;
        theme.background_color_ = Color::from_bytes(30, 30, 30);
        theme.text_color_ = Color::from_bytes(220, 220, 220);
        theme.highlight_color_ = Color::from_bytes(0, 120, 215);
        theme.button_background_color_ = Color::from_bytes(55, 55, 55);
        theme.button_text_color_ = Color::from_bytes(220, 220, 220);
        theme.button_border_color_ = Color::from_bytes(80, 80, 80);
        theme.button_border_radius_ = Px(4);
        theme.frame_background_color_ = Color::from_bytes(40, 40, 40);
        theme.frame_titlebar_color_ = Color::from_bytes(55, 55, 55);
        ui->set_config(theme);

        // Title
        auto title = ui->new_widget_as_label("My Game", 640, 150);
        title->set_font_size(48);
        title->set_text_color(Color::WHITE);
        title->set_text_alignment(TEXT_ALIGN_CENTER);

        // Menu frame
        auto menu_frame = ui->new_widget_as_frame("", 640, 300, 300, 250);
        menu_frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
        menu_frame->set_space_between(15);
        menu_frame->set_padding(10);

        // Play button
        auto play_btn = ui->new_widget_as_button("Play", 0, 0);
        play_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        play_btn->set_width(280);
        play_btn->signal_activated().connect([this]() {
            scenes->load_and_activate("game", SCENE_CHANGE_BEHAVIOUR_UNLOAD);
        });
        menu_frame->pack_child(play_btn);

        // Quit button
        auto quit_btn = ui->new_widget_as_button("Quit", 0, 0);
        quit_btn->set_resize_mode(RESIZE_MODE_FIXED_WIDTH);
        quit_btn->set_width(280);
        quit_btn->signal_activated().connect([this]() {
            app->shutdown();
        });
        menu_frame->pack_child(quit_btn);

        // Render layer
        auto layer = compositor->create_layer(this, camera(ui_camera_));
        layer->set_priority(RENDER_PRIORITY_FOREGROUND);
        link_pipeline(layer);
    }

private:
    CameraID ui_camera_;
};

// -----------------------------------------------------------
// Game Scene
// -----------------------------------------------------------
class GameScene : public Scene {
public:
    GameScene(Window* window) : Scene(window) {}

    void on_load() override {
        ui_camera_ = create_child<Camera2D>()->id();

        hud_ = create_child<GameHUD>();

        // Render layer
        auto layer = compositor->create_layer(this, camera(ui_camera_));
        layer->set_priority(RENDER_PRIORITY_FOREGROUND);
        link_pipeline(layer);

        // Simulate game state changes
        hud_->update_score(1500);
        hud_->update_lives(2);
        hud_->update_health(0.65f);
    }

    void on_update(float dt) override {
        Scene::on_update(dt);

        // Press Escape to return to menu
        auto input = window->input;
        if (input->is_button_down(BUTTON_ESCAPE)) {
            scenes->load_and_activate("menu", SCENE_CHANGE_BEHAVIOUR_UNLOAD);
        }
    }

private:
    CameraID ui_camera_;
    GameHUD* hud_ = nullptr;
};

// -----------------------------------------------------------
// Application
// -----------------------------------------------------------
class UIDemo : public Application {
public:
    UIDemo(const AppConfig& config) : Application(config) {}

private:
    bool init() override {
        scenes->register_scene<MenuScene>("menu");
        scenes->register_scene<GameScene>("game");
        scenes->register_scene<MenuScene>("main");
        scenes->activate("menu");
        return true;
    }
};

int main(int argc, char* argv[]) {
    _S_UNUSED(argc);
    _S_UNUSED(argv);

    AppConfig config;
    config.title = "UI Demo";
    config.width = 1280;
    config.height = 720;
    config.fullscreen = false;
    config.log_level = LOG_LEVEL_DEBUG;

    UIDemo app(config);
    return app.run(argc, argv);
}
```

---

## Best Practices

### 1. Always use a separate Camera2D for UI

```cpp
// Good
auto ui_camera = create_child<Camera2D>();
auto layer = compositor->create_layer(this, camera(ui_camera));
layer->set_priority(RENDER_PRIORITY_FOREGROUND);

// Bad -- rendering UI in 3D space
// auto layer = compositor->create_layer(this, perspective_camera);
```

### 2. Use Frames instead of manual positioning

Manual positioning is fragile. Frames handle layout automatically:

```cpp
// Good
auto frame = ui->new_widget_as_frame("", x, y, w, h);
frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM);
frame->pack_child(button1);
frame->pack_child(button2);

// Bad -- brittle manual positioning
// button1->move_to(100, 100);
// button2->move_to(100, 150);
```

### 3. Only update widgets when values change

```cpp
// Bad -- updating every frame
void on_update(float dt) override {
    score_label_->set_text("Score: " + std::to_string(score));
}

// Good -- update only when the value changes
void on_score_changed(int new_score) {
    score_label_->set_text("Score: " + std::to_string(new_score));
}
```

### 4. Use localization for translatable text

```cpp
auto label = ui->new_widget_as_label(_T("Play Game"), x, y);
auto button = ui->new_widget_as_button(_T("Settings"), x, y);
```

---

## Summary

| Concept | Key Methods |
|---------|------------|
| Create 2D camera | `create_child<Camera2D>()` |
| Create label | `ui->new_widget_as_label("Text", x, y)` |
| Create button | `ui->new_widget_as_button("Text", x, y)` |
| Create progress bar | `ui->new_widget_as_progress_bar(x, y, width, height)` |
| Create frame | `ui->new_widget_as_frame("Title", x, y, width, height)` |
| Pack widget into frame | `frame->pack_child(widget)` |
| Handle button click | `button->signal_activated().connect(...)` |
| Set font size | `widget->set_font_size(24)` |
| Set color | `widget->set_text_color(Color::WHITE)` |
| Set progress | `progress_bar->set_value(0.75f)` |
| Apply theme | `ui->set_config(theme)` |
| Layout direction | `frame->set_layout_direction(LAYOUT_DIRECTION_TOP_TO_BOTTOM)` |

**Next:** [Tutorial 5 -- Animation](05_animation.md)
