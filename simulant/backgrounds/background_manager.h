#pragma once

#include "../generic/property.h"
#include "../utils/unicode.h"
#include "background.h"
#include "../generic/manual_manager.h"

namespace smlt {

class Window;

class BackgroundManager {
public:
    BackgroundManager(Window* window);
    ~BackgroundManager();

    BackgroundPtr new_background(BackgroundType type);
    BackgroundPtr new_background_as_scrollable_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0);
    BackgroundPtr new_background_as_animated_from_file(const unicode& filename);

    BackgroundPtr background(BackgroundID bid);
    bool has_background(BackgroundID bid) const;
    BackgroundPtr delete_background(BackgroundID bid);
    uint32_t background_count() const;

    void delete_all_backgrounds();

    void update(float dt);

    Property<BackgroundManager, Window> window = { this, &BackgroundManager::window_ };

    void clean_up();
private:
    Window* window_;

    ManualManager<Background, BackgroundID> backgrounds_;
};

}
